#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define VERSION "0.3.0"
#define _FILE_OFFSET_BITS 64

const char SYNC_HEADER[12] = {
    (char) 0x00,
    (char) 0xFF,
    (char) 0xFF,
    (char) 0xFF,
    (char) 0xFF,
    (char) 0xFF,
    (char) 0xFF,
    (char) 0xFF,
    (char) 0xFF,
    (char) 0xFF,
    (char) 0xFF,
    (char) 0x00
};

const char SYNC_HEADER_MDF_AUDIO[12] = {
    (char) 0x80,
    (char) 0x80,
    (char) 0x80,
    (char) 0x80,
    (char) 0x80,
    (char) 0x80,
    (char) 0x80,
    (char) 0xC0,
    (char) 0x80,
    (char) 0x80,
    (char) 0x80,
    (char) 0x80
};

const char SYNC_HEADER_MDF[12] = {
    (char) 0x80,
    (char) 0xC0,
    (char) 0x80,
    (char) 0x80,
    (char) 0x80,
    (char) 0x80,
    (char) 0x80,
    (char) 0xC0,
    (char) 0x80,
    (char) 0x80,
    (char) 0x80,
    (char) 0x80
};

const char ISO_9660[8] = {
    (char) 0x01,
    (char) 0x43,
    (char) 0x44,
    (char) 0x30,
    (char) 0x30,
    (char) 0x31,
    (char) 0x01,
    (char) 0x00
};

void toc_file(char *destfilename, int sub) {
    char destfiletoc[1024], destfiledat[1024];
    FILE *ftoc;
    strcpy(destfiletoc, destfilename);
    strcpy(destfiledat, destfilename);
    strcpy(destfiletoc + strlen(destfilename) - 4, ".toc");
    strcpy(destfiledat + strlen(destfilename) - 4, ".dat");

    if ((ftoc = fopen(destfiletoc, "w")) != NULL) {
        fprintf(ftoc, "CD_ROM\n");
        fprintf(ftoc, "// Track 1\n");
        fprintf(ftoc, "TRACK MODE1_RAW");
        if (sub == 1)
            fprintf(ftoc, " RW_RAW\n");
        else
            fprintf(ftoc, "\n");
        fprintf(ftoc, "NO COPY\n");
        fprintf(ftoc, "DATAFILE \"%s\"\n", destfiledat);
        rename(destfilename, destfiledat);
        printf("Create TOC File : %s\n", destfiletoc);
        fclose(ftoc);
    } else {
        printf("%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int number_file(char *destfilename) {
    int i = 1, test_mdf = 0;
    int n_mdf;
    char mdf[8], destfilemdf[2354];
    FILE *fsource;
    strcpy(destfilemdf, destfilename);
    strcpy(destfilemdf + strlen(destfilename) - 1, ".0");
    for (i = 0; test_mdf == 0; i++) {
        if ((fsource = fopen(destfilemdf, "rb")) != NULL) {
            printf("\nCheck : ");
            snprintf(mdf, sizeof(mdf), "md%d", i);
            strcpy(destfilemdf + strlen(destfilename) - 3, mdf);
            printf("%s, ", destfilemdf);
            fclose(fsource);
        } else {
            test_mdf = 1;
        }
    }
    printf("\r                                   \n");
    n_mdf = i - 1;
    return (n_mdf);
}

void cuesheets(char *destfilename, int sector_data) {
    char destfilecue[1024], destfilebin[1024];
    FILE *fcue;
    strcpy(destfilecue, destfilename);
    strcpy(destfilebin, destfilename);
    strcpy(destfilecue + strlen(destfilename) - 4, ".cue");
    strcpy(destfilebin + strlen(destfilename) - 4, ".bin");
    fcue = fopen(destfilecue, "w");
    fprintf(fcue, "FILE \"%s\" BINARY\n", destfilebin);
    fprintf(fcue, "TRACK 1 MODE1/%d\n", sector_data);
    fprintf(fcue, "INDEX 1 00:00:00\n");
    rename(destfilename, destfilebin);
    printf("Create Cuesheets : %s\n", destfilecue);
    fclose(fcue);
}

void main_percent(int percent_bar) {
    int progress_bar, progress_space;
    printf("%d%% [:", percent_bar);
    for (progress_bar = 1; progress_bar <= (int)(percent_bar / 5); progress_bar++)
        printf("=");
    printf(">");
    for (progress_space = 0; progress_space < (20 - progress_bar); progress_space++)
        printf(" ");
    printf(":]\r");
}

void usage() {
    printf("mdf2iso v%s by Salvatore Santagati\n", VERSION);
    printf("Web     : http//mdf2iso.berlios.de\n");
    printf("Email   : salvatore.santagati@gmail.com\n");
    printf("Irc     : irc.freenode.net #ignus\n");
    printf("Note    : iodellavitanonhocapitouncazzo\n");
    printf("License : released under the GNU GPL v2 or later\n\n");
    printf("Usage :\n");
    printf("mdf2iso [OPTION] [BASENAME.MDF] [DESTINATION]\n\n");
    printf("OPTION\n");
    printf("\t--toc    Generate toc file\n");
    printf("\t--cue    Generate cue file\n");
    printf("\t--help   display this notice\n\n");
}

int main(int argc, char **argv) {
    int seek_ecc, sector_size, seek_head, sector_data, n_mdf;
    int cue = 0, cue_mode = 0, sub = 1, toc = 0, sub_toc = 0;
    int opts = 0;
    double size_iso, write_iso;
    long percent = 0;
    long i, source_length, progressbar;
    char buf[2448], destfilename[2354];
    FILE *fdest, *fsource;

    if (argc < 2) {
        usage();
        exit(EXIT_FAILURE);
    } else {
        for (i = 0; i < argc; i++) {
            if (!strcmp(argv[i], "--help")) {
                usage();
                exit(EXIT_SUCCESS);
            }
            if (!strcmp(argv[i], "--cue")) {
                cue = 1;
                opts++;
            }
            if (!strcmp(argv[i], "--toc")) {
                toc = 1;
                opts++;
            }
        }

        if ((cue == 1) && (toc == 1)) {
            usage();
            exit(EXIT_FAILURE);
        }

        if ((opts == 1) && (argc <= 2)) {
            usage();
            exit(EXIT_FAILURE);
        }

        if (argc >= (3 + opts))
            strcpy(destfilename, argv[2 + opts]);
        else {
            strcpy(destfilename, argv[1 + opts]);
            if (strlen(argv[1 + cue]) < 5 || strcmp(destfilename + strlen(argv[1 + opts]) - 4, ".mdf")) {
                strcpy(destfilename + strlen(argv[1 + opts]), ".iso");
            } else
                strcpy(destfilename + strlen(argv[1 + opts]) - 4, ".iso");
        }

        if ((fsource = fopen(argv[1 + opts], "rb")) != NULL) {
            fseek(fsource, 32768, SEEK_CUR);
            fread(buf, sizeof(char), 8, fsource);
            if (memcmp(ISO_9660, buf, 8)) {
                fseek(fsource, 0L, SEEK_SET);
                fread(buf, sizeof(char), 12, fsource);
                if (!memcmp(SYNC_HEADER, buf, 12)) {
                    fseek(fsource, 0L, SEEK_SET);
                    fseek(fsource, 2352, SEEK_CUR);
                    fread(buf, sizeof(char), 12, fsource);
                    if (!memcmp(SYNC_HEADER_MDF, buf, 12)) {
                        if (cue == 1) {
                            cue_mode = 1;
                            seek_ecc = 96;
                            sector_size = 2448;
                            sector_data = 2352;
                            seek_head = 0;
                        } else if (toc == 0) {
                            seek_ecc = 384;
                            sector_size = 2448;
                            sector_data = 2048;
                            seek_head = 16;
                        } else {
                            seek_ecc = 0;
                            sector_size = 2448;
                            sector_data = 2448;
                            seek_head = 0;
                            sub_toc = 1;
                        }
                    } else {
                        if (!memcmp(SYNC_HEADER, buf, 12)) {
                            if (cue == 1) {
                                cue_mode = 1;
                                sub = 0;
                                seek_ecc = 0;
                                sector_size = 2352;
                                sector_data = 2352;
                                seek_head = 0;
                            }
                            if (toc == 0) {
                                seek_ecc = 288;
                                sector_size = 2352;
                                sector_data = 2048;
                                seek_head = 16;
                            } else {
                                seek_ecc = 0;
                                sector_size = 2352;
                                sector_data = 2352;
                                seek_head = 0;
                            }
                        } else {
                            printf("Sorry I don't know this format :(\n");
                            exit(EXIT_FAILURE);
                        }
                    }
                } else {
                    fseek(fsource, 0L, SEEK_SET);
                    fseek(fsource, 2352, SEEK_CUR);
                    fread(buf, sizeof(char), 12, fsource);
                    if (memcmp(SYNC_HEADER_MDF_AUDIO, buf, 12)) {
                        printf("Sorry I don't know this format :(\n");
                        exit(EXIT_FAILURE);
                    } else {
                        seek_head = 0;
                        sector_size = 2448;
                        seek_ecc = 96;
                        sector_data = 2352;
                        cue = 0;
                    }
                }

                if ((fdest = fopen(destfilename, "wb")) != NULL) {
                } else {
                    printf("%s\n", strerror(errno));
                    exit(EXIT_FAILURE);
                };

                fseek(fsource, 0L, SEEK_END);
                source_length = ftell(fsource) / sector_size;
                size_iso = (int)(source_length * sector_data);
                progressbar = 100 / source_length;
                fseek(fsource, 0L, SEEK_SET);

                int cur_percent = -1;
                for (i = 0; i < source_length; i++) {
                    fseek(fsource, seek_head, SEEK_CUR);
                    if (fread(buf, sizeof(char), sector_data, fsource)) {
                    } else {
                        printf("%s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    };
                    if (fwrite(buf, sizeof(char), sector_data, fdest)) {
                    } else {
                        printf("%s\n", strerror(errno));
                        exit(EXIT_FAILURE);
                    };
                    fseek(fsource, seek_ecc, SEEK_CUR);
                    write_iso = (int)(sector_data * i);
                    if (i != 0)
                        percent = (int)(write_iso * 100 / size_iso);
                    if (cur_percent != percent) {
                        main_percent(percent);
                        cur_percent = percent;
                    }
                }

                printf("100%%[:====================:]\n");

                fclose(fsource);
                fclose(fdest);

                if (cue == 1)
                    cuesheets(destfilename, sector_data);
                if (toc == 1)
                    toc_file(destfilename, sub_toc);
                if ((toc == 0) && (cue == 0))
                    printf("Create iso9660: %s\n", destfilename);

                exit(EXIT_SUCCESS);
            } else {
                printf("This is file iso9660 ;)\n");
            }

            n_mdf = number_file(destfilename) - 1;
            fclose(fsource);
            exit(EXIT_SUCCESS);
        } else {
            printf("%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        };
    }
}
