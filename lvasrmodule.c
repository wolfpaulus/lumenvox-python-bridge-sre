////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  lvasr Grammar-based ASR Interface for Python
//  Version 1.0.0
//  Author: Wolf Paulus, wolf@wolfpaulus.com
//
//	Description :
//	This is a simple bridge between python apps and the LumenVox ASR Server.
//  A python statement like this:
// 		text= lv.asr("./mygrammar.grxml,"./myrecording.raw")
//	would use the ASRCLIENT to access the LumenVox ASR server to load the provided grammar and decode the raw audio
//  and eventually return text and confidence scrore(s)
//  Learn more about Python Extension Programming with C at
//       http://www.tutorialspoint.com/python/python_further_extensions.htm
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Python.h> // Python.h must be included before any standard headers are included.
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <syslog.h>
#include <LV_SRE.h>


#ifdef WIN32
#define stricmp _stricmp
#else //linux
#define stricmp strcasecmp
#endif

#define VOICE_CHANNEL 1

//
// declare local C functions
//
void DoInit(void);
void DoUninit(void);

void PrintUsage(const char *prog_name);
int DoLoadGrammar(const char * grammar_label, const char * grammar_file );
int DoUnloadGrammar(const char * grammar_label );
const char * DoDecode(const char * grammar_label, const char * grammar_file, const char * audio_file, int * error_code);

//
// declare global variables
//
HPORT port = NULL;

//
//  Python-Interface for the decode function.
//  Decode is called with a
//    label string, which may refer to an already loaded grammar with that label
//    file name of the GrXML grammar to be used during the decoding (can be None)
//    filename of the raw audio file
//
//  To be called from python like so:
//	  import lvasr
// 	  text= lvasr.decode("myGrammarName","./myGrammar.grxml","myRecording.raw")
//
//  Returns the text or NULL/None
//
static PyObject* lvasr_decode(PyObject* self, PyObject* args) {
	const char *label;
	const char *grammarfile = NULL;
	const char *soundfile;
	const char *result = NULL;
    int error_code;
    char errormsg[160];

	if (!PyArg_ParseTuple(args, "szs", &label, &grammarfile, &soundfile)) {
		if(!PyErr_Occurred()) {
            PyErr_SetString(PyExc_TypeError,"You must supply three arguments: Label, GrammarFileName (maybe None), RawVoiceSound");
        }
	} else {
		result= DoDecode(label, grammarfile, soundfile, &error_code);
		if (result==NULL) {
		    strcpy(errormsg,"<error>");
		    strcat(errormsg,LV_SRE_ReturnErrorString(error_code));
		    strcat(errormsg,"</error>");
		}
	}
	return result!=NULL ? Py_BuildValue("s",result) : Py_BuildValue("s", errormsg);
}

//
//  Python-Interface for loading and activating a grammar.
//  loadgrammer is called with a
//    label string, which can be used later to refer to the loaded grammar
//    file name of the GrXML grammar that needs to be loaded and activated
//
//  To be called from python like so:
//	  import lvasr
// 	  text= lvasr.loadgrammar("myGrammarName","./myGrammar.grxml")
//
//  Returns 0, if everything worked well.
//
static PyObject* lvasr_loadgrammar(PyObject* self, PyObject* args) {
	const char *label;
	const char *grammarfile;
	int result = -1;
	
	if (!PyArg_ParseTuple(args, "ss", &label, &grammarfile)) {
		if(!PyErr_Occurred()) {
            PyErr_SetString(PyExc_TypeError,"You must supply two argument: Label, GrammarFileName");
        }
	} else {
	    result = DoLoadGrammar(label, grammarfile);
	}	
	return Py_BuildValue("i",result);
}

//
//  Python-Interface for unloading/de-activating a grammar.
//  unloadgrammer is called with a
//    label string, the string that was used to load and activate it.
//
//  To be called from python like so:
//	  import lvasr
// 	  text= lvasr.unloadgrammar("myGrammarName")
//
//  Returns 0, if everything worked well.
//
static PyObject* lvasr_unloadgrammar(PyObject* self, PyObject* args) {
	const char *label;
	int result = -1;

	if (!PyArg_ParseTuple(args, "s", &label)) {
		if(!PyErr_Occurred()) {
            PyErr_SetString(PyExc_TypeError,"You must supply an argument: Label");
        }
	} else {
	    result = DoUnloadGrammar(label);
	}
	return Py_BuildValue("i",result);
}

//
//  Python-Interface for checking if the server is available.
//  To be called from python like so:
//	  import lvasr
// 	  k= lvasr.serveravailable()
//
//  Returns 0 if none of the listed servers are reachable.
//
static PyObject* lvasr_serveravailable(PyObject* self, PyObject* args) {
	int result = LV_SRE_IsServerAvailable();
	return Py_BuildValue("i",result);
}

//
//  Python-Interface for converting an error code into an error message.
//  To be called from python like so:
//	  import lvasr
// 	  text= lvasr.errormessage(22)
//
//  Returns A null-terminated static string describing the error code
//  if the error_code is not a valid error code, the string "Invalid Error Code" is returned.
//
static PyObject* lvasr_errormessage(PyObject* self, PyObject* args) {
	int err_code = 0;
	if (!PyArg_ParseTuple(args, "i", &err_code)) {
		if(!PyErr_Occurred()) {
            PyErr_SetString(PyExc_TypeError,"You must supply an argument: error_code");
        }
	}
	return Py_BuildValue("s",LV_SRE_ReturnErrorString(err_code));
}

//
// Python-Interface to (re-) initialize the connection to the ASR server
static PyObject* lvasr_init(PyObject* self, PyObject* args) {
    DoInit();
    return Py_BuildValue("s",NULL);
}

//
// Python-Interface to uninitialize the connection to the ASR server
static PyObject* lvasr_uninit(PyObject* self, PyObject* args) {
    DoUninit();
    return Py_BuildValue("s",NULL);
}


//
//  Python-Interface
//	Method mapping table, an array of PyMethodDef structures
//  This table needs to be terminated with a sentinel that consists of NULL and 0 values for the appropriate members.
//
static PyMethodDef module_methods[] = {
	{ "decode", (PyCFunction)lvasr_decode, METH_VARARGS, "Decode an raw audio message into text" },
    { "loadgrammar", (PyCFunction)lvasr_loadgrammar, METH_VARARGS, "Load/Precompile a GrXML or BNF grammar" },
    { "unloadgrammar", (PyCFunction)lvasr_unloadgrammar, METH_VARARGS, "Unload a GrXML or BNF grammar" },
    { "serveravailable", (PyCFunction)lvasr_serveravailable, METH_VARARGS, "Checks if the ASR server is up" },
    { "errormessage", (PyCFunction)lvasr_errormessage, METH_VARARGS, "Convert error code into message" },
    { "init", (PyCFunction)lvasr_init, METH_VARARGS, "(Re-)initialize the SpeechPort" },
    { "uninit", (PyCFunction)lvasr_uninit, METH_VARARGS, "Uninitialize the SpeechPort" },
	{ NULL, NULL, 0, NULL }
};

//
//  Python-Interface
//	This function is called by the Python interpreter when the module is loaded.
//
PyMODINIT_FUNC initlvasr(void) {
	(void) Py_InitModule("lvasr", module_methods);
}

//
//  DoInit()
//
void DoInit() {
    int result = 0;

    LV_SRE_Startup();
    openlog("lvasrmodulelog", LOG_PID|LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "initlvasr");

	if (port==NULL) {
	    syslog(LOG_INFO, "DoInit entered with port is NULL");
        port = LV_SRE_CreateClient(&result, NULL, NULL, 6); // log verbosity 0..6
		syslog(LOG_INFO, "LV_SRE_CreateClient : %d", result);
        syslog(LOG_INFO, "LV_SRE_CreateClient : %s", LV_SRE_ReturnErrorString(result));
    }
    syslog(LOG_INFO, "DoInit, port: %d", port);
}


//
//  DoUninit()
//
void DoUninit() {
    int result = 0;
    if (port!=NULL) {
        result = LV_SRE_DestroyClient( port );
        syslog(LOG_INFO, "LV_SRE_DestroyClient : %d", result);
        port=NULL;
    }
    LV_SRE_Shutdown();
    syslog(LOG_INFO, "DoUnInit called LV_SRE_Shutdown()");
    closelog();
}

//
// DoLoadGrammar loads the given GrXML (or BNF) grammar file and assigns it the given label.
//
int DoLoadGrammar(const char * grammar_label, const char * grammar_file ) {
	int result = 0;	
	if (port != NULL) {
		result = LV_SRE_LoadGrammar(port, grammar_label, grammar_file);
		syslog(LOG_INFO, "LV_SRE_LoadGrammar : %d", result);
		if (LV_SUCCESS==result) {
		    result = LV_SRE_ActivateGrammar(port, grammar_label);
    		syslog(LOG_INFO, "LV_SRE_ActivateGrammar : %d", result);
		}
	} else {
	    syslog(LOG_ERR, "DoLoadGrammar, port is NULL");
	}
	return result;
}


//
// DoUnloadGrammar unloads the given GrXML (or BNF) grammar.
//
int DoUnloadGrammar(const char * grammar_label ) {
	int result = 0;
	if (port != NULL) {
		result = LV_SRE_DeactivateGrammar(port, grammar_label);
		syslog(LOG_INFO, "LV_SRE_DeactivateGrammar : %d", result);
	} else {
	    result = LV_FAILURE;
	    syslog(LOG_ERR, "DoUnloadGrammar, port is NULL");
	}
	return result;
}

//
// DoDecode decodes the given raw audiofile
//
const char * DoDecode(const char * grammar_label, const char * grammar_file, const char * audio_file, int * error_code) {
	const enum SOUND_FORMAT audio_format = PCM_16KHZ;
	unsigned char * audio_buffer = NULL;
	FILE * fp = NULL;
    const char * interpretation = NULL;
    int audio_file_length = -1;
	int NumberOfNBestFromASR = 0;
	int IndexOfTheCurrentNBest = 0;
	int NumberOfInterpretationsFromASR = 0;
	int IndexOfTheCurrentInterpretation = 0;

    int result = LV_SUCCESS;

    do {
	    if (port == NULL) {
		    result = LV_FAILURE;
		    syslog(LOG_INFO, "DoDecode entered with port is NULL");
		    break;
	    }

	    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	    //	Read the audio into a buffer
	    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	    fp = fopen(audio_file, "rb");
	    if (fp == NULL) {
		    printf("Error opening audio file : %s\n", audio_file);
		    result = LV_FAILURE;
	        break;
	    }
	    if ( fseek(fp, 0, SEEK_END) != 0 ) {
		    printf("fseek encountered a problem while traversing the audio\n");
        }

	    audio_file_length = ftell(fp);
        rewind(fp);
	    audio_buffer = malloc(audio_file_length);
	    if ( fread(audio_buffer, sizeof(char), (size_t)audio_file_length, fp) != (size_t)audio_file_length ) {
		    printf("fread encountered a problem while reading the audio into a buffer\n");
        }
	    if ( fclose(fp) != 0 ) {
		    printf("fclose encountered a problem while trying to close\n");
		}
	    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //	loading the audio data into the voice channel of the speech port. This is not the recommended
        //	way of feeding audio to the speech port however for the purposes of this example we are keeping it simple.
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    	result = LV_SRE_LoadVoiceChannel(port, VOICE_CHANNEL, audio_buffer, audio_file_length, audio_format);
   		syslog(LOG_INFO, "LV_SRE_LoadVoiceChannel : %d", result);
	    if (result != LV_SUCCESS) {
		    break;
	    }

	    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    	//	We are checking for less than LV_SUCCESS because decode will return a positive number based on
    	//	how many interpretations it gets. A negative number returned is an error code.
	    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		syslog(LOG_INFO, "port value beforeLV_SRE_Decode : %d", port);
		result = LV_SRE_Decode(port, VOICE_CHANNEL, LV_ACTIVE_GRAMMAR_SET, LV_DECODE_BLOCK );
        syslog(LOG_INFO, "LV_SRE_Decode : %d", result);
        if (result < LV_SUCCESS) {
            result = LV_SRE_GetLastDecodeError(port,VOICE_CHANNEL);
            syslog(LOG_INFO, "LV_SRE_GetLastDecodeError : %d", result);
            syslog(LOG_INFO, "LV_SRE_GetLastDecodeError : %s", LV_SRE_ReturnErrorString(result));
		    break;
	    }

	    NumberOfNBestFromASR = LV_SRE_GetNumberOfNBestAlternatives(port, VOICE_CHANNEL);
        syslog(LOG_INFO, "LV_SRE_GetNumberOfNBestAlternatives : %d", NumberOfNBestFromASR);
	    if (NumberOfNBestFromASR == 0) {
		    result = LV_NO_RESULT_AVAILABLE;
		    break;
    	}

        // short cut for loops
        interpretation = LV_SRE_GetInterpretationString(port, VOICE_CHANNEL, 0);
        break; // only get one

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        //	here we will cycle through the possible NBest results and print the interpretations to screen
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        for (IndexOfTheCurrentNBest = 0; IndexOfTheCurrentNBest < NumberOfNBestFromASR; ++IndexOfTheCurrentNBest) {

            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            //	We use this function to switch to the NBest at a certain position in the list.
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            result = LV_SRE_SwitchToNBestAlternative(port, VOICE_CHANNEL, IndexOfTheCurrentNBest);
            syslog(LOG_INFO, "LV_SRE_SwitchToNBestAlternative : %d", result);
            if (result != LV_SUCCESS) {
                continue;
            }

            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            //	Now we get the number of interpretations for the specified NBest result
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            NumberOfInterpretationsFromASR = LV_SRE_GetNumberOfInterpretations(port, VOICE_CHANNEL);
            syslog(LOG_INFO, "LV_SRE_GetNumberOfInterpretations : %d", NumberOfInterpretationsFromASR);
            if (NumberOfInterpretationsFromASR == 0) {
                continue;
            }

            for (IndexOfTheCurrentInterpretation = 0; IndexOfTheCurrentInterpretation < NumberOfInterpretationsFromASR; ++IndexOfTheCurrentInterpretation) {
                interpretation = LV_SRE_GetInterpretationString(port, VOICE_CHANNEL, IndexOfTheCurrentInterpretation);
                break; // only get one
            }
            break; // only get one
        }
    } while (false);
    if (audio_buffer!=NULL) {
        free(audio_buffer);
    }
    *error_code = result;
	return interpretation;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Prints Usage information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PrintUsage(const char *prog_name)
{
	printf("Usage: %s <grammar-label> <grammar-file> [<audio-file>]\n", prog_name);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	main
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char * argv[])
{
	char * grammar_label = NULL;
	char * grammar_file = NULL;
	char * audio_file = NULL;
    const char * text = NULL;
	int result = 0;

    DoInit();

	if (0==LV_SRE_IsServerAvailable()) {
        printf("Can not connect to any ASR servers.");
	    fflush(stdout);
	   	DoUninit();
		return 1;

	}

    printf("LV_SRE_CreateClient : %s\n", LV_SRE_ReturnErrorString(result));

	if (argc < 2 || argc > 4) {
		PrintUsage(argv[0]);
	    DoUninit();
		return -1;
	}

	grammar_label = argv[1];
	grammar_file = argv[2];

	result = DoLoadGrammar("MyGrammar", grammar_file);
    printf("DoLoadGrammar : %s\n", LV_SRE_ReturnErrorString(result));
    printf("Decoding ...\n");
	if (argc == 4) {
		audio_file = argv[3];
		text = DoDecode("MyGrammar", NULL, audio_file, &result);
		printf("DoDecode Result: %s\n", LV_SRE_ReturnErrorString(result));
		printf("Interpretation String: %s\n",text);
    }
	DoUninit();
	fflush(stdout);

	return 0;
}