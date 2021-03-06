/*******************************************************************************

  File:    sqgetimgtfp.c
  Project: SETIkit
  Authors: Rob Ackermann <ackermann at seti dot org>

  Copyright 2011 The SETI Institute

  SETIkit is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  SETIkit is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with SETIkit.  If not, see <http://www.gnu.org/licenses/>.

  Implementers of this code are requested to include the caption
  "Licensed through SETI" with a link to setiQuest.org.

  For alternate licensing arrangements, please contact
  The SETI Institute at www.seti.org or setiquest.org.

*******************************************************************************/

#ifndef __x86_64__
#define _FILE_OFFSET_BITS 64
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <math.h>

#include <sq_utils.h>
#include <sq_imaging.h>
#include <sq_constants.h>
#define MAXVAL MAX_PIXEL_VAL

//          1         2         3         4         5         6         7
// 123456789012345678901234567890123456789012345678901234567890123456789012
char *usage_text[] = {
    "                                                                        ",
    "NAME                                                                    ",
    "  sqgetimgtfp - internal utility called by sqwaterfalls                 ",
    "SYNOPSIS                                                                ",
    "  sqgetimgtfp [OPTIONS] -time-freq-pwr.dat                              ",
    "DESCRIPTION                                                             ",
    "  -c  integer (required), channel                                       ",
    "  -o  integer (required), offset (0 through 7)                          ",
    "                                                                        "
};
int arrlen = sizeof(usage_text)/sizeof(*usage_text);

#define TFPW 8388608
#define CHANW (TFPW / 4096)
#define IMGW (CHANW / 8)

#define STATW 5

#define MAXROWS 340

signed int chan = 0;
unsigned int ofst = 0;

FILE *tfp;

float imgd[MAXROWS*(STATW*IMGW)];

void power_scale(unsigned int rowN);

int main(int argc, char *argv[]) {
    unsigned int rowi, rowN;

    uint64_t rowofst;

    int opt;

    unsigned char flags = 0x00;

    while ((opt = getopt(argc, argv, "c:o:")) != -1) {
        switch (opt) {
        case 'c':
            sscanf(optarg, "%d", &chan);
            flags |= 0x01;
            break;
        case 'o':
            sscanf(optarg, "%u", &ofst);
            flags |= 0x02;
            break;
        }
    }

    if ((!(flags == 0x03)) || (!((argc - optind) == 1))) {
        print_usage(usage_text, arrlen);
        exit(EXIT_FAILURE);
    }

    tfp = fopen(argv[optind], "rb");
    if (!(tfp)) {
        fprintf(stderr, "unable to open file %s", argv[optind]);
        exit(EXIT_FAILURE);
    }

    rowi = 0;
    rowofst = (TFPW/2)+(-(CHANW/2))+(chan*CHANW)+(ofst*IMGW)+(-((STATW/2)*IMGW));
    for (rowi = 0; rowi < MAXROWS; rowi++) 
    {
        if (!(fseek(tfp, ((((uint64_t)(rowi*TFPW))+rowofst)*sizeof(float)), SEEK_SET) == 0))
        {
            fprintf(stderr, "Could not seek anymore.\n");
            break;
        }
            
        if (!(fread(&imgd[rowi*(STATW*IMGW)], sizeof(float), (STATW*IMGW), tfp) == (STATW*IMGW)))
        {
            fprintf(stderr, "Could not read anymore, on row %d.\n", rowi);
            break;
        }
    }
    rowN = rowi;

    power_scale(rowN);

    for (rowi = 0; rowi < rowN; rowi++) {
        fwrite(&imgd[(rowi*(STATW*IMGW))+((STATW/2)*IMGW)], sizeof(float), IMGW, stdout);
    }

    exit(EXIT_SUCCESS);
}

void power_scale(unsigned int rowN) {
    unsigned int imgi;

    float imgvalf;

    float mean, stddev;
    float min, max;

    unsigned int imgN = rowN*(STATW*IMGW);

    mean = 0.0;
    for (imgi = 0; imgi < imgN; imgi++)
        mean += sqrt(imgd[imgi]);
    mean /= (float) imgN;

    stddev = 0.0;
    for (imgi = 0; imgi < imgN; imgi++)
        stddev += (sqrt(imgd[imgi]) - mean) * (sqrt(imgd[imgi]) - mean);
    stddev = sqrt(stddev / (float) imgN);

    min = mean - stddev;
    if (min < 0.0) min = 0.0;
    min = min * min;
    max = mean + (2.3 * stddev);
    max = max * max;

    for (imgi = 0; imgi < imgN; imgi++) {
        imgvalf = imgd[imgi];
        imgvalf -= min;
        imgvalf *= ((float) MAXVAL) / (max - min);
        imgd[imgi] = imgvalf;
    }
}
