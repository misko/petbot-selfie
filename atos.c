//
//  main.c
//  DeepBeliefSDK
//
//  Created by Peter Warden on 4/28/14.
//  Copyright (c) 2014 Jetpac, Inc. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <libjpcnn.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>

float* predictions;
int predictionsLength;
char** predictionsLabels;
int predictionsLabelsLength;
const char * networkFileName;
int layer;
const char * predictorFileName;
const char * imageFileName;


int release=0;
sem_t stopped;
sem_t running;

void * analyze() {


  void * networkHandle = jpcnn_create_network(networkFileName);
  if (networkHandle == NULL) {
    fprintf(stderr, "DeepBeliefSDK: Couldn't load network file '%s'\n", networkFileName);
    return;
  }
	  void * predictor = jpcnn_load_predictor(predictorFileName);
	  if (predictor==NULL) {
		fprintf(stderr,"Failed to load predictor\n");
		return;
	  }
  while (1>0) {
	  sem_wait(&running);

	  while (1>0) {
	
		
		if (release==1) {
			break;
		}	

		fprintf(stderr, "taking picture\n");
		//fswebcam here
		unlink(imageFileName); //remove the file if it exists
		int pid=fork();
		if (pid==0) {
			//child
			//TODO make sure acquired image file
			char * args[] = { "/usr/bin/fswebcam","-r","640x480","--skip","10",
				" --no-underlay","--no-info","--no-banner","--no-timestamp","--quiet",imageFileName, NULL };
			int r = execv(args[0],args);
			fprintf(stderr,"SHOULD NEVER REACH HERE %d\n",r);
		}
		//master
		wait(NULL);	


		//next load image
		if (release==1) {
			break;
		}	

		  void * imageHandle = jpcnn_create_image_buffer_from_file(imageFileName);
		  if (imageHandle == NULL) {
		    fprintf(stderr, "DeepBeliefSDK: Couldn't load image file '%s'\n", imageFileName);
		    continue;
		  }
		
		//next classify
		if (release==1) {
			break;
		}	


		  jpcnn_classify_image(networkHandle, imageHandle, 0, layer, &predictions, &predictionsLength, &predictionsLabels, &predictionsLabelsLength);
		
		//next predict
		if (release==1) {
			break;
		}	
		  float pred = jpcnn_predict(predictor, predictions, predictionsLength);
		  fprintf(stdout,"%f\n",pred);

		//next send out the image if it passes
		  if (pred>0.15 && release==0) {

			char pred_s[1024];
			sprintf(pred_s,"%0.4f", pred);
			int pid=fork();
			if (pid==0) {
				//child
				char * args[] = { "/bin/bash","./send_atos.sh",imageFileName, pred_s, NULL };
				int r = execv(args[0],args);
				fprintf(stderr,"SHOULD NEVER REACH HERE %d\n",r);
			}
			//master
			wait(NULL);	
		  }
		  jpcnn_destroy_image_buffer(imageHandle);
	  }




	  sem_post(&stopped);

  }
	  jpcnn_destroy_predictor(predictor);
	 jpcnn_destroy_network(networkHandle);

}

int main(int argc, const char * argv[]) {


  


  if (argc!=5) {
	fprintf(stderr,"%s network_filename layer svm_filename img_filename\n",argv[0]);
	exit(1);
  }

  networkFileName=argv[1];
  layer = atoi(argv[2]);
  predictorFileName=argv[3];
  imageFileName=argv[4];


        sem_init(&stopped,0,0);
        sem_init(&running,0,0);

        pthread_t analyze_thread;
        int  iret1;


	release=1;

        //start the tcp_client
        iret1 = pthread_create( &analyze_thread, NULL, analyze, NULL);
        if(iret1) {
                fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
                exit(EXIT_FAILURE);
        }


	//monitor STDIN/ STDOUT
	char buffer[1024];
	while (1>0) {
		fgets(buffer, 1024, stdin);
		if (strlen(buffer)>0 ) {
			buffer[strlen(buffer)-1]='\0';
		}
		fprintf(stderr, "GOT %s\n",buffer);
		if (strcmp(buffer,"STOP")==0) {
			if (release==1) {
				//hmmmm	
			} else {
				release=1;
				sem_wait(&stopped); //wait for other guy to stop
				//when here other guy is stopped
				fprintf(stdout,"STOPPED\n");	
			}
		} else if (strcmp(buffer,"GO")==0) {
		fprintf(stderr, "GOT GO %s\n",buffer);
			if (release==0) {

			} else {
				release=0;	
				sem_post(&running);
			}	
		}
	}	

  return 0;
}
