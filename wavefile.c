/******************************************************************************
* A complete framework for reading, processing then writing WAV files
******************************************************************************/
#include <stdio.h>
#include <malloc.h>
#include "wavefile.h"

static int  four_bytes;
static int  two_bytes;
static char four_chars[4];
/******************************************************************************
* Check that four bytes match what was expected
******************************************************************************/
static int check_four_bytes(FILE *f, char test[]) {
        int c;
        int i;
        for(i = 0; i < 4; i++) {
                c = getc(f);
                if(c == EOF) {
                        printf("Expected a character\n");
                        return 0;
                }

                if(c != test[i]) {
                        printf("Not what I was expecting in char %i\n", i);
                        return 0;
                }
        }
        return 1;
}

/******************************************************************************
* Read for characters from the input file
******************************************************************************/
static int read_four_chars(FILE *f) {
        int c;
        c = getc(f);
        if(c == EOF) {
                printf("Expected to read byte0 of four\n");
                return 0;
        }
        four_chars[0] = c;

        c = getc(f);
        if(c == EOF) {
                printf("Expected to read byte 1 of four\n");
                return 0;
        }
        four_chars[1] = c;

        c = getc(f);
        if(c == EOF) {
                printf("Expected to read byte 2 of four\n");
                return 0;
        }
        four_chars[2] = c;

        c = getc(f);
        if(c == EOF) {
                printf("Expected to read byte 3 of four\n");
                return 0;
        }
        four_chars[3] = c;

        return 1;
}

/******************************************************************************
* Write four characters to the output file
******************************************************************************/
static int write_four_chars(FILE *f, char *data) {
        int c;
        if(putc(data[0],f)==EOF)     return 0;
        if(putc(data[1],f)==EOF)     return 0;
        if(putc(data[2],f)==EOF)     return 0;
        if(putc(data[3],f)==EOF)     return 0;
        return 1;
}

/******************************************************************************
* Read in four four-byte value (low byte first)
******************************************************************************/
static int read_four_bytes(FILE *f) {
        int c;
        four_bytes = 0;
        c = getc(f);
        if(c == EOF) {
                printf("Expected to read byte0 of four\n");
                return 0;
        }
        four_bytes += ((unsigned char)c);

        c = getc(f);
        if(c == EOF) {
                printf("Expected to read byte 1 of four\n");
                return 0;
        }
        four_bytes += ((unsigned char)c)*256;

        c = getc(f);
        if(c == EOF) {
                printf("Expected to read byte 2 of four\n");
                return 0;
        }
        four_bytes += ((unsigned char)c)*256*256;

        c = getc(f);
        if(c == EOF) {
                printf("Expected to read byte 3 of four\n");
                return 0;
        }
        four_bytes += ((char)c)*256*256*256;

        return 1;
}

/******************************************************************************
* write out a four byte value, low byte first
******************************************************************************/
static int write_four_bytes(FILE *f, int i) {
        int c;
        char data[4];
        data[0] = i;
        data[1] = i/256;
        data[2] = i/256/256;
        data[3] = i/256/256/256;
        return write_four_chars(f,data);
}

/******************************************************************************
* Read in a two-byte value, low byte first
******************************************************************************/
static int read_two_bytes(FILE *f) {
        int c;
        two_bytes = 0;
        c = getc(f);
        if(c == EOF) {
                printf("Expected to read byte0 of two\n");
                return 0;
        }
        two_bytes += ((unsigned char)c);

        c = getc(f);
        if(c == EOF) {
                printf("Expected to read byte 1 of two\n");
                return 0;
        }
        two_bytes += ((char)c)*256;

        return 1;
}

/******************************************************************************
* Write out two characters to the file
******************************************************************************/
static int write_two_chars(FILE *f, char *data) {
        int c;
        if(putc(data[0],f)==EOF)     return 0;
        if(putc(data[1],f)==EOF)     return 0;
        return 1;
}

/******************************************************************************
* Write out a two-byte value, low byte first
******************************************************************************/
static int write_two_bytes(FILE *f, int i) {
        int c;
        char data[2];
        data[0] = i;
        data[1] = i>>8;
        return write_two_chars(f,data);
}
/******************************************************************************
* Read in the 'RIFF' file details (just type and total file size)
******************************************************************************/
static int read_in_riff_header(struct wave *w, FILE *f) {
  /*****************************
  * Look for the RIFF header   *
  *****************************/
  if(!check_four_bytes(f,"RIFF")) {
        printf("expecting RIFF\n");
        return 0;
  }
  printf("Found 'RIFF'\n");

  /******************************
  * Now get the size            *
  ******************************/
  if(!read_four_bytes(f)) {
         printf("Invalid size\n");
         return 0;
  }
  w->total_size = four_bytes;
  printf("Total file is %i, plus 8 bytes header\n",w->total_size);
  return 1;
}

/******************************************************************************
* Ignore the 'LIST' data block in the WAV file
******************************************************************************/
static int read_in_list_header(struct wave *w, FILE *f) {
  int list_size;

  /******************************
  * Now get the size            *
  ******************************/
  if(!read_four_bytes(f)) {
         printf("Invalid size\n");
         return 0;
  }
  list_size = four_bytes;
  printf("List size is %i, plus 8 bytes header\n", list_size);

  while(list_size > 0) {
        int c;
        c = fgetc(f);
        if(c == EOF) {
                printf("List block ended early\n");
                return 0;
        }
        list_size--;
  }
  return 1;
}

/******************************************************************************
* Read in the WAV file header block
******************************************************************************/
static int read_in_wave_header(struct wave *w, FILE *f)
{
  int block_len;
  /********************************
  * Look for the 'fmt\0' header   *
  ********************************/
  if(!check_four_bytes(f,"fmt ")) {
        printf("expecting 'fmt '\n");
        return 0;
  }
  printf("Found 'fmt '\n");

  /********************************
  * Look for the header size      *
  ********************************/
  if(!read_four_bytes(f)) {
         printf("Invalid header size\n");
         return 0;
  }
  printf("Header size is %i\n",four_bytes);
  block_len = four_bytes;
  if(block_len < 16 || block_len > 100) {
          printf("Unexpected headeer size %i\n", four_bytes);
          return 0;
  }

  /********************************
  * Look for the Audio format     *
  ********************************/
  if(!read_two_bytes(f)) {
         printf("Invalid data type\n");
         return 0;
  }
  if(two_bytes != 1) {
          printf("Unexpected data type  %i\n", two_bytes);
          return 0;
  }
  printf("Data type is 1\n");

  /**********************************
  * Look for the Number of channels *
  **********************************/
  if(!read_two_bytes(f)) {
         printf("Invalid channel count\n");
         return 0;
  }
  w->channel_count = two_bytes;
  if(w->channel_count != 2 && w->channel_count != 1) {
          printf("Unexpected channel count %i\n", w->channel_count);
          return 0;
  }
  printf("Channel count is %i\n", w->channel_count);

  /********************************
  * Look for the sample rate      *
  ********************************/
  if(!read_four_bytes(f)) {
         printf("Invalid sample_rate\n");
         return 0;
  }
  w->sample_rate = four_bytes;
  if(w->sample_rate > 96000) {
          printf("Unexpected sample_rate %i\n", w->sample_rate);
          return 0;
  }
  printf("Sample rate is %i\n",w->sample_rate);

  /********************************
  * Look for the data rate        *
  ********************************/
  if(!read_four_bytes(f)) {
         printf("Invalid data rate\n");
         return 0;
  }
  w->data_rate = four_bytes;
  printf("Data rate is %i\n", w->data_rate);

  /********************************
  * Look for the block align      *
  ********************************/
  if(!read_two_bytes(f)) {
         printf("Invalid Block align\n");
         return 0;
  }
  w->block_align = two_bytes;
  printf("Block align is %i\n", w->block_align);

  /********************************
  * Look for the bits per sample  *
  ********************************/
  if(!read_two_bytes(f)) {
         printf("Invalid bits per sample\n");
         return 0;
  }
  w->bits_per_sample = two_bytes;
  printf("Bits per sample is %i\n", w->bits_per_sample);

  /* Wow! All done! */
  while(block_len > 16) {
        getc(f);
        block_len--;
  }
  return 1;
}

/******************************************************************************
* Allocate memory and read in the data block of a WAV file
******************************************************************************/
static int read_in_samples(struct wave *w, FILE *f)
{
  int data_size;
  int c, n_channels = w->channel_count;
  /******************************
  * Now get the size            *
  ******************************/
  if(!read_four_bytes(f)) {
         printf("Invalid size\n");
         return 0;
  }
  data_size = four_bytes;
  printf("'data' size is %i, plus 8 bytes header\n", data_size);

  w->channel_data = malloc(sizeof(int *) * n_channels); 
  if(w->channel_data == NULL) {
    printf("Out of memory for channel data array\n");
    return 0;
  }
  for(c = 0; c < n_channels; c++) {
     w->channel_data[c]  = NULL;
  }

  for(c = 0; c < n_channels; c++) {
     w->channel_data[c]  = malloc(sizeof(int)*(data_size/2)/n_channels);
     if(w->channel_data[c] == NULL) {
        printf("Out of memory for channel data\n");
        return 0;
     }
  }

  if(c != n_channels) {
     for(c = 0; c < n_channels; c++) {
        if(w->channel_data[c] == NULL) {
           free(w->channel_data[c]);
        }
     }
     free(w->channel_data);
     w->channel_data = NULL;
     return 0;
  }

  w->sample_count = 0;
  while(data_size >= 2*n_channels) {
     for(c = 0; c < n_channels; c++) {
        if(!read_two_bytes(f)) {
           printf("Run out of data with %i bytes left to read\n", data_size);
           return 0;
        }
        w->channel_data[c][w->sample_count] = two_bytes;
        data_size -= 2;
     }
     w->sample_count++;
  }
  printf("Just read in %i samples\n", w->sample_count);
  return 1;
}

/******************************************************************************
* Read in and validate the input wave file
******************************************************************************/
struct wave *wavefile_read(char *filename) {
  FILE *f;
  char c;
  int data_read_flag;
  struct wave *w;

  f = fopen(filename,"rb");
  if(f == NULL) {
          printf("Unable to open file\n");
          return NULL;
  }

  w = malloc(sizeof(struct wave));
  if(w == NULL) {
          printf("Out of memory\n");
          return NULL;
  }

  if(!read_in_riff_header(w,f)) {
        fclose(f);
        free(w);
        return NULL;
 }

 data_read_flag = 0;
 while(data_read_flag == 0 && read_four_chars(f)) {
        if( four_chars[0] == 'W' && four_chars[1] == 'A' && four_chars[2] == 'V' && four_chars[3] == 'E') {
                if(!read_in_wave_header(w, f)) {
                        fclose(f);
                        return 0;
                }
        } else if( four_chars[0] == 'L' && four_chars[1] == 'I' && four_chars[2] == 'S' && four_chars[3] == 'T') {
                if(!read_in_list_header(w, f)) {
                        fclose(f);
                        return 0;
                }
        } else if( four_chars[0] == 'd' && four_chars[1] == 'a' && four_chars[2] == 't' && four_chars[3] == 'a') {
                if(!read_in_samples(w, f)) {
                        fclose(f);
                        return 0;
                }
                data_read_flag = 1;
        } else {
          printf("Unknown chunk id '%c%c%c%c'\n",four_chars[0], four_chars[1], four_chars[2], four_chars[3]);
          fclose(f);
          return 0;
        }
  }
  fclose(f);
  printf("Wow! Have read in the file!\n");
  return w;
}

/******************************************************************************
* Write out the header to the supplied file handle
******************************************************************************/
static int write_out_headers(struct wave *w, FILE *f) {
        int total_length;

        total_length = 8                    /* RIFF section length  */
                     + 8 + 16               /* WAVE section length */
                                 + 8 + w->sample_count*2 * w->channel_count;  /* Data section length */

        if(!write_four_chars(f,"RIFF")) {
                printf("Error writing RIFF\n");
                return 0;
        }
        if(!write_four_bytes(f,total_length)) {
                printf("Error writing total length\n");
                return 0;
        }
        if(!write_four_chars(f,"WAVE")) {
                printf("Error writing WAVE\n");
                return 0;
        }

        if(!write_four_chars(f,"fmt ")) {
                printf("Error writng 'fmt '\n");
                return 0;
        }

        if(!write_four_bytes(f,16)) {
                printf("Error writing header size\n");
                return 0;
        }

        if(!write_two_bytes(f,1)) {
                printf("Error writing audio format\n");
                return 0;
        }

        if(!write_two_bytes(f,w->channel_count)) {
                printf("Error writing channel count\n");
                return 0;
        }

        if(!write_four_bytes(f,w->sample_rate)) {
                printf("Error writing sample rate\n");
                return 0;
        }

        if(!write_four_bytes(f,w->data_rate)) {
                printf("Error writing data rate\n");
                return 0;
        }

        if(!write_two_bytes(f,w->block_align)) {
                printf("Error writing block align\n");
                return 0;
        }

        if(!write_two_bytes(f,w->bits_per_sample)) {
                printf("Error writing bits_per_sample\n");
                return 0;
        }
        return 1;
}

/******************************************************************************
* Write out the sample values to the supplied file handle
******************************************************************************/
static int write_out_samples(struct wave *w, FILE *f) {
        int data_length;
        int i;
        int has_clipped = 0;
        int n_channels = w->channel_count;

        data_length = w->sample_count*2*n_channels;

        if(!write_four_chars(f,"data")) {
                printf("Error writing 'data'\n");
                return 0;
        }
        if(!write_four_bytes(f,data_length)) {
                printf("Error writing data length\n");
                return 0;
        }
        for(i = 0; i < w->sample_count; i++) {
            int c;
            for(c = 0; c < n_channels; c++) {
                int clipped = w->channel_data[c][i];
                if (clipped < -32767) {
                        clipped = -32767;
                        has_clipped++;
                } else if(clipped > 32767) {
                        clipped = 32767;
                        has_clipped++;
                }
                if(!write_two_bytes(f, clipped)) {
                        printf("Error writing sample\n");
                }
            }
        }
        if(has_clipped > 0) {
                printf("WARNING: %i samples have been clampped as they are too loud\n",has_clipped);
        }
        return 1;
}

/******************************************************************************
* Check that we will not clip the signal, and if needed adjust the volume
******************************************************************************/
void prevent_clipping(struct wave *w) {
    int i,c;
    int max = 0, min=0;
    int n_channels = w->channel_count;

    for(c = 0; c < n_channels; c++) { 
        for(i = 0; i < w->sample_count; i++) {
            if(w->channel_data[c][i] < min) min = w->channel_data[c][i];
            if(w->channel_data[c][i] > max) max = w->channel_data[c][i];
        }
    }
    if(min <= -32767 && max >= 32767) {
        printf("Signal will not clip - no scaling needed\n");
    }
    else {
        /* Which one do we need to scale to fit? */
        if(max < -min) {
             max = -min;
        }
        max = (max*3)/4;
        printf("Signal will be scaled by 8191/%i\n",max);

        for(c = 0; c < n_channels; c++) { 
            for(i = 0; i < w->sample_count; i++) {
               w->channel_data[c][i]  = w->channel_data[c][i]  * 8191/max;
            }
        }
    }
}
/******************************************************************************
* Write the header, then the data for the output file
******************************************************************************/
int wavefile_write(struct wave *w, char *filename) {
        FILE *f;
        printf("Preventing clipping\n");
//      prevent_clipping(w);
        f = fopen(filename, "wb");
        if(f == NULL) {
                printf("Unable to open output file '%s'",filename);
                return 0;
        }
        printf("Output file '%s' opened\n",filename);
        if (!write_out_headers(w, f)) {
                fclose(f);
                return 0;
        }
        printf("Have written headers\n");

        if (!write_out_samples(w, f)) {
                fclose(f);
                return 0;
        }
        printf("Have written data\n");
        fclose(f);
        return 1;
}

/******************************************************************************
* Release all the memory holding the details of the wave file
******************************************************************************/
void wavefile_destroy(struct wave *w) {
    int c;
    if(w->channel_data != NULL) {
      for(c= 0; c < w->channel_count; c++) {
         if(w->channel_data[c] != NULL)
            free(w->channel_data[c]);
      }
      free(w->channel_data);
    }
    free(w);
}

/******************************************************************************
* Create the data for a new wave file
******************************************************************************/
struct wave *wavefile_new(int sample_rate, int sample_count, int channel_count) {
    struct wave *w;
    int i,c;

    printf("Creating an empty wave structure for %i samples\n",sample_count);
    w = malloc(sizeof(struct wave));
    if(w == NULL) {
        printf("Out of memory\n");
        return NULL;
    } 

    w->channel_count   = channel_count;
    w->sample_rate     = sample_rate;
    w->data_rate       = sample_rate * 2 * channel_count;
    w->block_align     = 2 * channel_count;
    w->bits_per_sample = 16;
    w->sample_count    = sample_count;
 
    w->channel_data = malloc(sizeof(int *)*channel_count);
    if(w->channel_data == NULL) {
        printf("Out of memory\n");
        free(w);
        return NULL;
    }

    for(c= 0; c < channel_count; c++) {
        w->channel_data[c] = NULL;
    }

    for(c= 0; c < channel_count; c++) {
        w->channel_data[c] = malloc(sizeof(int)*sample_count);
        if(w->channel_data[c] == NULL)
            break;
    }

    if(c != channel_count) {
        printf("Out of memory\n");
        for(c= 0; c < channel_count; c++) {
             if(w->channel_data[c] != NULL)
                free(w->channel_data[c]);
        }
        free(w->channel_data);
        free(w);
        return NULL;
    }

    for(c= 0; c < channel_count; c++) {
       for(i = 0; i < sample_count; i++) {
             w->channel_data[c][i] = 0;
       }
    }
    return w;
}
/********************** end of program ****************************************/
