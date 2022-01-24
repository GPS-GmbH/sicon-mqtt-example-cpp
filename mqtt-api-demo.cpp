#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTAsync.h"
#include "jsmn.h"

#if !defined(_WIN32)
#include <unistd.h>
#else
#include <windows.h>
#endif

#if defined(_WRS_KERNEL)
#include <OsWrapper.h>
#endif

#define ADDRESS     "tcp://192.168.1.1:1883"
#define CLIENTID    "ExampleClientSub"
#define TOPIC       "device/16/postReport"
 // The device reporting config defines which Y value has the leakage rate
#define REPORTINGIDENTIFIER_EJEKTOR1       "Y_1"
#define REPORTINGIDENTIFIER_EJEKTOR2       "Y_2"
#define REPORTINGIDENTIFIER_EJEKTOR3       "Y_3"
#define REPORTINGIDENTIFIER_EJEKTOR4       "Y_4"
#define REPORTINGIDENTIFIER_COUNTER1       "X_1"
#define REPORTINGIDENTIFIER_COUNTER2       "X_2"
#define REPORTINGIDENTIFIER_COUNTER3       "X_3"
#define REPORTINGIDENTIFIER_COUNTER4       "X_4"

#define QOS         1

// global state

int disc_finished = 0;
int subscribed = 0;
int finished = 0;

// event handlers

void onConnect(void* context, MQTTAsync_successData* response) {
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	// code returned by various mqtt functions to check for errors
	int returnCode;

	printf("Successful connection\n");

	printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
		"Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
	opts.context = client;
	if ((returnCode = MQTTAsync_subscribe(client, TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", returnCode);
		finished = 1;
	}
}

void onConnectFailure(void* context, MQTTAsync_failureData* response) {
	printf("Connect failed, returnCode %d\n", response->code);
	finished = 1;
}

void onConnectionLost(void* context, char* cause) {
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int returnCode;

	printf("\nConnection lost\n");
	if (cause)
		printf("     cause: %s\n", cause);

	printf("Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	if ((returnCode = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", returnCode);
		finished = 1;
	}
}

void onDisconnect(void* context, MQTTAsync_successData* response) {
	printf("Successful disconnection\n");
	disc_finished = 1;
}

void onDisconnectFailure(void* context, MQTTAsync_failureData* response) {
	printf("Disconnect failed, returnCode %d\n", response->code);
	disc_finished = 1;
}

// helper function to compare json keys
static int jsoneq(const char* json, jsmntok_t* tok, const char* s) {
	if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
		strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

int onMessageArrive(void* context, char* topicName, int topicLen, MQTTAsync_message* message) {
	jsmn_parser p;
	jsmntok_t t[128]; /* We expect no more than 128 JSON tokens */
	jsmn_init(&p);
	printf("Message arrived\n");
	printf("     topic: %s\n", topicName);
	char* payload = (char*)message->payload;
	printf("   message: %.*s\n", message->payloadlen, payload);
	int r = jsmn_parse(&p, payload, strlen(payload), t, 128);
	for (int i = 1; i < r; i++) {
		if (jsoneq(payload, &t[i], REPORTINGIDENTIFIER_EJEKTOR1) == 0) {
			printf("leakagerate E1: %.*s mbar/s\n", t[i + 1].end - t[i + 1].start, payload + t[i + 1].start);
		}
		if (jsoneq(payload, &t[i], REPORTINGIDENTIFIER_EJEKTOR2) == 0) {
			printf("leakagerate E2: %.*s mbar/s\n", t[i + 1].end - t[i + 1].start, payload + t[i + 1].start);
		}
		if (jsoneq(payload, &t[i], REPORTINGIDENTIFIER_EJEKTOR3) == 0) {
			printf("leakagerate E3: %.*s mbar/s\n", t[i + 1].end - t[i + 1].start, payload + t[i + 1].start);
		}
		if (jsoneq(payload, &t[i], REPORTINGIDENTIFIER_EJEKTOR4) == 0) {
			printf("leakagerate E4: %.*s mbar/s\n", t[i + 1].end - t[i + 1].start, payload + t[i + 1].start);
		}
		if (jsoneq(payload, &t[i], REPORTINGIDENTIFIER_COUNTER1) == 0) {
			printf("Global Cycle Counter E1: %.*s pcs\n", t[i + 1].end - t[i + 1].start, payload + t[i + 1].start);
		}
		if (jsoneq(payload, &t[i], REPORTINGIDENTIFIER_COUNTER2) == 0) {
			printf("Global Cycle Counter E2: %.*s pcs\n", t[i + 1].end - t[i + 1].start, payload + t[i + 1].start);
		}
		if (jsoneq(payload, &t[i], REPORTINGIDENTIFIER_COUNTER3) == 0) {
			printf("Global Cycle Counter E3: %.*s pcs\n", t[i + 1].end - t[i + 1].start, payload + t[i + 1].start);
		}
		if (jsoneq(payload, &t[i], REPORTINGIDENTIFIER_COUNTER4) == 0) {
			printf("Global Cycle Counter E4: %.*s pcs\n", t[i + 1].end - t[i + 1].start, payload + t[i + 1].start);
		}
		// if (jsoneq(payload, &t[i], REPORTINGIDENTIFIER_EJEKTOR1) == 0) {
		// 	printf("leakagerate: %.*s mbar/s\n", t[i + 1].end - t[i + 1].start, payload + t[i + 1].start);
		// }
	}

	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);
	return 1;
}

// setup

int main(int argc, char* argv[])
{
	MQTTAsync client;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
	disc_opts.onSuccess = onDisconnect;
	disc_opts.onFailure = onDisconnectFailure;
	int returnCode;
	int terminalInputChar;

	// Create a new client
	if ((returnCode = MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL))
		!= MQTTASYNC_SUCCESS)
	{
		// validation of options (address, clientid) resulted in an error
		printf("Failed to create client, return code %d\n", returnCode);
		returnCode = EXIT_FAILURE;
		return returnCode;
	}
	
	conn_opts.context = client;

	if ((returnCode = MQTTAsync_setCallbacks(client, client, onConnectionLost, onMessageArrive, NULL)) != MQTTASYNC_SUCCESS)
	{
		// validation of callbacks resulted in an error
		printf("Failed to set callbacks, return code %d\n", returnCode);
		returnCode = EXIT_FAILURE;
		MQTTAsync_destroy(&client);
	}

	// actual connection to the broker
	if ((returnCode = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", returnCode);
		returnCode = EXIT_FAILURE;
		MQTTAsync_destroy(&client);
	}

	while (!subscribed && !finished)
#if defined(_WIN32)
		Sleep(100);
#else
		usleep(10000L);
#endif

	if (finished) return returnCode;
	
	// wait for user input loop to abort the program
	do {
		terminalInputChar = getchar();
	} while (terminalInputChar != 'Q' && terminalInputChar != 'q');

	// user aborted - now gracefully disconnect and free the client
	if ((returnCode = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start disconnect, return code %d\n", returnCode);
		returnCode = EXIT_FAILURE;
		MQTTAsync_destroy(&client);
	}
	while (!disc_finished)
	{
#if defined(_WIN32)
		Sleep(100);
#else
		usleep(10000L);
#endif
	}
}
