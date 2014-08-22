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

#define NETWORK_FILE_NAME "jetpac.ntwk"

int main(int argc, const char * argv[]) {


  float* predictions;
  int predictionsLength;
  char** predictionsLabels;
  int predictionsLabelsLength;
  int index;

  if (argc!=5) {
	fprintf(stderr,"%s network_filename layer svm_filename img_filename\n",argv[0]);
	exit(1);
  }

  const char * networkFileName=argv[1];
  int layer = atoi(argv[2]);
  const char * predictorFileName=argv[3];
  const char * imageFileName=argv[4];


  void * networkHandle = jpcnn_create_network(networkFileName);
  if (networkHandle == NULL) {
    fprintf(stderr, "DeepBeliefSDK: Couldn't load network file '%s'\n", networkFileName);
    return 1;
  }

  void * predictor = jpcnn_load_predictor(predictorFileName);
  if (predictor==NULL) {
	fprintf(stderr,"Failed to load predictor\n");
	return 1;
  }


  int i=0;
  while (i>-1) {
	fprintf(stderr, "taking picture\n");
  	//fswebcam here
	int pid=fork();
	if (pid==0) {
		//child
		char * args[] = { "/usr/bin/fswebcam","-r","640x480","--skip","10",
			" --no-underlay","--no-info","--no-banner","--no-timestamp","--quiet",imageFileName, NULL };
		int r = execv(args[0],args);
		fprintf(stderr,"SHOULD NEVER REACH HERE %d\n",r);
	}
	//master
	wait(NULL);	


	  void * imageHandle = jpcnn_create_image_buffer_from_file(imageFileName);
	  if (imageHandle == NULL) {
	    fprintf(stderr, "DeepBeliefSDK: Couldn't load image file '%s'\n", imageFileName);
	    return 1;
	  }



	  jpcnn_classify_image(networkHandle, imageHandle, 0, layer, &predictions, &predictionsLength, &predictionsLabels, &predictionsLabelsLength);
	  float pred = jpcnn_predict(predictor, predictions, predictionsLength);
	  fprintf(stdout,"%f\n",pred);
	  if (pred>0.15) {

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
	i++;
  }

  jpcnn_destroy_predictor(predictor);
  /*for (index = 0; index < predictionsLength; index += 1) {
    float predictionValue;
    char* label = predictionsLabels[index];
    predictionValue = predictions[index];
    fprintf(stdout, "%d\t%f\t%s\n", index, predictionValue, label);
  }*/
  

  jpcnn_destroy_network(networkHandle);

  return 0;
}
