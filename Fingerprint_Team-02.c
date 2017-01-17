/**
 * Simple C-Tool that performes the simple matching algorithm. 
 * The -h flag can be used to toggle the use of the Hough-Transformation
 * The -p flag is used to determine a probe image in the xyt format
 * The -g flag is used to determine the gallery, either a single image
 *         or a complete directory with .xyt files
 * The -s flag can be used to test either a single file (when set) in this
 *         case the -g flag is the path of a single .xyt file, or (when not set)
         a whole directory. In this case the -g flag is the path to the dir. 
 * 
 * ************ ASSIGNMENT ***************
 * Implement the functions loadMinutiae, getScore and alignment.
 * The function documentation of the functions, and the exercise 
 * descriptions, contain more information !
 *
 * DO NOT CHANGE ANYTHING IN THE OTHER FUNCTIONS! 
 * BUT YOU ARE FREE TO ADD ADDITIONAL FUNCTIONS.
 * 
 * THE PROGRAMM HAS TO BE COMPILED WITH GCC ON A LINUX SYSTEM VIA:
 *         gcc -O2 simple_matcher.c -lm -o simple_matcher
 * IT CAN BE EXECUTED VIA:
 *         ./simple_matcher -p images/xxx_x.xyt -g images [-h]
 * ************ ASSIGNMENT ***************
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#define MAX_MINUTIAE    130        /* should be ajusted if a file has more minutiae */
#define A_X             400        /* used for Array in alignment, should be */
#define A_Y             500        /* adjusted if out of boundaries error occurs*/
#define threshold_d      14        /* for getScore */
#define threshold_r      18        /* for getScore */
#define thres_t          18        /* for alignment */
#define PI                3.14159

struct xyt_struct {
    int nrows;
    int xcol[MAX_MINUTIAE];
    int ycol[MAX_MINUTIAE];
    int thetacol[MAX_MINUTIAE];
};

struct node {
    struct xyt_struct data;
    char* filepath;
    int score;
    struct node *next;
};

void print_usage(char*);
void test_single(char*, char*, int);
void test_multiple(char*, char*, int);
struct xyt_struct alignment(struct xyt_struct probe, struct xyt_struct gallery);
struct xyt_struct loadMinutiae(const char *xyt_file);
int getScore(struct xyt_struct probe, struct xyt_struct gallery);

int main(int argc, char ** argv) {
    /* Shall Hough Transformation be used? */
    int hflag = 0;
    /* If set only one file is tested and galleryname is used a file name
       If not set, galleryname is used as a directory name and all files in
       this directory are compared to the probe image */
    int sflag = 0;
    /* String to the probe-image */
    char* probename = NULL;
    /* String to the gallery-image */
    char* galleryname = NULL;
    /* Name of this programm - needed for error msg */
    char* thisfile = argv[0];
    /* ARGUMENT PARSING - START */
    if(argc < 4) {
        print_usage(thisfile);
        exit(1);
    }
    int c = 0;
    opterr = 0;
    while ((c = getopt(argc, argv, "p:g:hs")) != -1) {
        switch (c) {
            case 'p':
                probename = optarg;
                break;
            case 'g':
                galleryname = optarg;
                break;
            case 'h':
                hflag = 1;
                break;
            case 's':
                sflag = 1;
                break;
            case '?':
                if(optopt == 'p' || optopt == 'g') {
                    printf("Opion -%c requires an argument!\n", optopt);
                    print_usage(thisfile);
                    return 1;
                }
            default:
                print_usage(thisfile);
                return 1;
        }
    }
    if(probename[0] == '-') {
        printf("Opion -p requires an argument!\n");
        print_usage(thisfile);
        return 1;
    }
    if(galleryname[0] == '-') {
        printf("Opion -g requires an argument!\n");
        print_usage(thisfile);
        return 1;
    }
    /* ARGUMENT PARSING - DONE */
    /* Test Minutiae, either single file or whole directory */
    if (sflag) {
        test_single(probename, galleryname, hflag);
    } else {
        test_multiple(probename, galleryname, hflag);
    }
    return 0;
}

void print_usage(char* thisfile) {
    printf("USAGE: %s -p probe-image -g gallery-image [-h] [-s]\n", thisfile);
    printf("\t -p \t The probe image that has to be tested!\n");
    printf("\t -g \t Path to galery images!\n");
    printf("\t[-h]\t If set the Hough Transformation is used!\n");
    printf("\t[-s]\t If set only a single file is tested and -g takes filename!\n");
    printf("\n");
}

void test_single(char* probename, char* galleryname, int hflag) {
    /* Load .xyt images to struct */
    struct xyt_struct probe = loadMinutiae(probename);
    struct xyt_struct gallery = loadMinutiae(galleryname);
    /* If Hough Should be used */
    if(hflag) {
        gallery = alignment(probe, gallery);
    }
    /* Calculate the score between the two .xyt images */
    int score = getScore(probe,gallery);    
    printf("The score is: %d\n",score);
}

void test_multiple(char* probename, char* dirname, int hflag) {
    /* Needed for dir scanning */
    DIR *d;
    struct dirent *dir;
    /* Needed for linked list of files */
    struct node *root;    //Root node
    struct node *curr;    //Current node
    /* To be loaded path of file is composed in it */
    char* toload = malloc(256 * sizeof(char));
    /* Create a new root node */
    root = (struct node *) malloc(sizeof(struct node));
    root->next = NULL;
    curr = root;
    printf("Looking for images in dir: %s\n", dirname);
    d = opendir(dirname);
    if(d) {
        while((dir = readdir(d)) != NULL) {
            //Leave out '.' and '..'
            if(dir->d_name[0] == '.') {
                continue;
            }
            int len = strlen(dir->d_name);
            //If the filename ends with xyt load the image to our list
            if(dir->d_name[len - 3] == 'x' && dir->d_name[len - 2] == 'y' && dir->d_name[len - 1] == 't') {
                //Compose the path of the to be loaded image in toload
                snprintf(toload, 256 * sizeof(char), "%s/%s", dirname, dir->d_name);
                //Store pathname in the struct
                int path_len = strlen(toload);
                curr->filepath = malloc(path_len * sizeof(char));
                strcpy(curr->filepath, toload);
                //Store loaded Minutiae in single linked list and add new next node
                curr->data = loadMinutiae(toload);
                curr->next = (struct node *) malloc(sizeof(struct node));
                curr = curr->next;
                curr->filepath = NULL;
                //If the pointer returned by malloc was null we ran out of memory
                if(curr == NULL) {
                    printf("Out of memory!\n");
                    exit(1);
                }
                //Set next node to NULL so we have a sentinel at the end. 
                curr->next = NULL;
            }
        }
        //Close directory and free filepath composer when all files are loaded
        closedir(d);
        free(toload);
    }
    //Load the probe image
    struct xyt_struct probe = loadMinutiae(probename);
    //Set the current node back to root, so we can traverse the list again.
    curr = root;
    if(curr != NULL) {
        while(curr != NULL && curr->filepath != NULL) {
            /* If Hough Transformation shall be used */
            if(hflag) {
                curr->data = alignment(probe, curr->data);
            }
            curr->score = getScore(probe, curr->data);
            printf("Node: %p, Path: %s, Score: %d\n", curr, curr->filepath, curr->score);
            curr = curr->next;
        }
    } else {
        printf("The list is empty!\n");
        exit(1);
    }
}

/** 
 * TODO: Implement the Generalised Hough Transform
 * 
 * The probe image, is the image you want to align, the gallery image to. 
 * Return the aligned gallery xyt_struct as the result of this function.
 */
struct xyt_struct alignment(struct xyt_struct probe, struct xyt_struct galleryimage) {
    return galleryimage;
}

/**
 * TODO: Implement the simple Minutiae Pairing Algorithm
 * Compare the gallery image to the probe image and return
 * the comparison score as an integer. 
 */
int getScore(struct xyt_struct probe, struct xyt_struct galleryimage) {
    int score=0;

    char used_gallery[MAX_MINUTIAE] = { 0 };
    char used_probe[MAX_MINUTIAE]  = { 0 };

    //printf("matching gallery image with %d minutiae against probe image with %d minutiae\n", galleryimage.nrows, probe.nrows);

    int i,j;
    for(i = 0; i < galleryimage.nrows; i++)
    {
    	for(j = 0; j < probe.nrows; j++)
    	{
    		if(!used_gallery[i] && !used_probe[j])
    		{
    			int distance_spatial = (int)( floor( sqrt( pow(probe.xcol[j] - galleryimage.xcol[i], 2.0) + pow(probe.ycol[j] - galleryimage.ycol[i], 2.0) ) ) );
    			int distance_angle = abs(probe.thetacol[j] - galleryimage.thetacol[i]);
				#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
    			distance_angle = MIN(distance_angle, 360-distance_angle);
    			#undef MIN
    			if(distance_spatial < threshold_d && distance_angle < threshold_r)
    			{
	    			score++;
					used_gallery[i] = 1;
					used_probe[j] = 1;
    			}
    		}
    	}
    }

    return score;
}

/** 
 * TODO: Load minutiae from file (filepath is given as char *xyt_file) 
 * into the xyt_struct 'res' and return it as the result.
 *
 * Check for corrupted files, e.g. a line has less than 3, or more than 4
 * values, and fill the 'xcol', 'ycol' and 'thetacol' in the xyt_struct. 
 * At last set the 'numofrows' variable in the xyt_struct to the amount of 
 * loaded rows.
 */
struct xyt_struct loadMinutiae(const char *xyt_file) {
    struct xyt_struct res = {0};

    FILE* pFile = fopen(xyt_file, "r");
    if(pFile)
    {
        size_t i;
        char Line[100];

        for(i = 0; i < MAX_MINUTIAE && fgets(Line, 100, pFile) != NULL; i++)
        {
            int xcol, ycol, thetacol;
            if(sscanf(Line, "%d %d %d %*d", &xcol, &ycol, &thetacol) == 3)
            {
                res.xcol[i] = xcol;
                res.ycol[i] = ycol;
                res.thetacol[i] = thetacol;
            }
            else
            {
                // read an unexpected number of columns, abort.
                break;
            }
        }
        res.nrows = i;

        fclose(pFile);
    }

    return res;
}
