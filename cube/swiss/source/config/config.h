#ifndef CONFIG_H_
#define CONFIG_H_

#include "swiss.h"
typedef struct {
	char game_id[4];
	char padding_1[1];
	char game_name[64];
	char padding_2[1];
	char comment[128];
	char padding_3[1];
	char status[32];
	char padding_4[1];
	int gameVMode;
	int forceHScale;
	short forceVOffset;
	int forceVFilter;
	int disableDithering;
	int forceAnisotropy;
	int forceWidescreen;
	int forceEncoding;
	int invertCStick;
	int emulateAudioStreaming;
	int emulateReadSpeed;
} ConfigEntry __attribute__((aligned(32)));

void config_parse(char *configData);
void config_parse_swiss_settings(char *configData);
void config_find(ConfigEntry *entry);
int config_update(ConfigEntry *entry);
int config_create();
int config_init();
int config_get_count();
int config_update_file();
void config_load_current(ConfigEntry *config);
void config_unload_current();

#endif

