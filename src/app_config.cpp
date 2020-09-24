#include "emonesp.h"
#include "espal.h"
#include "mqtt.h"
#include "emoncms.h"
#include "input.h"
#include "app_config.h"

#define EEPROM_SIZE     4096
#define CHECKSUM_SEED    128

static int getNodeId()
{
  #ifdef ESP32
  unsigned long chip_id = ESP.getEfuseMac();
  #else
  unsigned long chip_id = ESP.getChipId();
  #endif
  DBUGVAR(chip_id);
  int chip_tmp = chip_id / 10000;
  chip_tmp = chip_tmp * 10000;
  DBUGVAR(chip_tmp);
  return (chip_id - chip_tmp);
}

String node_type = NODE_TYPE;
String node_description = NODE_DESCRIPTION;
int node_id = getNodeId();
String node_name_default = node_type + String(node_id);
String node_name = "";
String node_describe = "";

// Wifi Network Strings
String esid = "";
String epass = "";

// Web server authentication (leave blank for none)
String www_username = "";
String www_password = "";

// EMONCMS SERVER strings
String emoncms_server = "";
String emoncms_path = "";
String emoncms_node = "";
String emoncms_apikey = "";
String emoncms_fingerprint = "";

// MQTT Settings
String mqtt_server = "";
int mqtt_port = 1883;
String mqtt_topic = "";
String mqtt_user = "";
String mqtt_pass = "";
String mqtt_feed_prefix = "";

// Calibration Settings
String voltage_cal = "";
String ct1_cal = "";
String ct2_cal = "";
String freq_cal = "";
String gain_cal = "";
#ifdef SOLAR_METER
String svoltage_cal = "";
String sct1_cal = "";
String sct2_cal = "";
#endif

// Timer Settings
int timer_start1 = 0;
int timer_stop1 = 0;
int timer_start2 = 0;
int timer_stop2 = 0;

int voltage_output = 0;

String ctrl_mode = "Off";
bool ctrl_update = 0;
bool ctrl_state = 0;
int time_offset = 0;


uint32_t flags;

void config_changed(String name);

ConfigOptDefenition<uint32_t> flagsOpt = ConfigOptDefenition<uint32_t>(flags, 0, "flags", "f");

ConfigOpt *opts[] =
{
// Wifi Network Strings, 0
  new ConfigOptDefenition<String>(esid, "", "ssid", "ws"),
  new ConfigOptSecret(epass, "", "pass", "wp"),

// Web server authentication (leave blank for none), 2
  new ConfigOptDefenition<String>(www_username, "", "www_username", "au"),
  new ConfigOptSecret(www_password, "", "www_password", "ap"),

// Advanced settings, 4
  new ConfigOptDefenition<String>(node_name, node_name_default, "hostname", "hn"),

// EMONCMS SERVER strings, 5
  new ConfigOptDefenition<String>(emoncms_server, "emoncms.org", "emoncms_server", "es"),
  new ConfigOptDefenition<String>(emoncms_path, "", "emoncms_path", "ep"),
  new ConfigOptDefenition<String>(emoncms_node, node_name, "emoncms_node", "en"),
  new ConfigOptSecret(emoncms_apikey, "", "emoncms_apikey", "ea"),
  new ConfigOptDefenition<String>(emoncms_fingerprint, "", "emoncms_fingerprint", "ef"),

// MQTT Settings, 10
  new ConfigOptDefenition<String>(mqtt_server, "emonpi", "mqtt_server", "ms"),
  new ConfigOptDefenition<int>(mqtt_port, 1883, "mqtt_port", "mpt"),
  new ConfigOptDefenition<String>(mqtt_topic, "emonesp", "mqtt_topic", "mt"),
  new ConfigOptDefenition<String>(mqtt_user, "emonpi", "mqtt_user", "mu"),
  new ConfigOptSecret(mqtt_pass, "emonpimqtt2016", "mqtt_pass", "mp"),
  new ConfigOptDefenition<String>(mqtt_feed_prefix, "", "mqtt_feed_prefix", "mp"),

// Calibration settings
  new ConfigOptDefenition<String>(voltage_cal, "3920", "voltage_cal", "cv"),
  new ConfigOptDefenition<String>(ct1_cal, "39473", "ct1_cal", "ct1"),
  new ConfigOptDefenition<String>(ct2_cal, "39473", "ct2_cal", "ct2"),
  new ConfigOptDefenition<String>(freq_cal, "4485", "freq_cal", "cf"),
  new ConfigOptDefenition<String>(gain_cal, "21", "gain_cal", "cg"),
  #ifdef SOLAR_METER
  new ConfigOptDefenition<String>(svoltage_cal, "3920", "svoltage_cal", "scv"),
  new ConfigOptDefenition<String>(sct1_cal, "39473", "sct1_cal", "sct1"),
  new ConfigOptDefenition<String>(sct2_cal, "39473", "sct2_cal", "sct2"),
  #endif

// Timer Settings
  new ConfigOptDefenition<int>(timer_start1, 0, "timer_start1", "tsr1"),
  new ConfigOptDefenition<int>(timer_stop1, 0, "timer_stop1", "tsp1"),
  new ConfigOptDefenition<int>(timer_start2, 0, "timer_start2", "tsr2"),
  new ConfigOptDefenition<int>(timer_stop2, 0, "timer_stop2", "tsp2"),
  new ConfigOptDefenition<int>(time_offset, 0, "time_offset", "to"),

  new ConfigOptDefenition<int>(voltage_output, 0, "voltage_output", "vo"),

  new ConfigOptDefenition<String>(ctrl_mode, "Off", "ctrl_mode", "cm"),

// Flags, 23
  &flagsOpt,

// Virtual Options, 24
  new ConfigOptVirtualBool(flagsOpt, CONFIG_SERVICE_EMONCMS, CONFIG_SERVICE_EMONCMS, "emoncms_enabled", "ee"),
  new ConfigOptVirtualBool(flagsOpt, CONFIG_SERVICE_MQTT, CONFIG_SERVICE_MQTT, "mqtt_enabled", "me"),
  new ConfigOptVirtualBool(flagsOpt, CONFIG_CTRL_UPDATE, CONFIG_CTRL_UPDATE, "ctrl_update", "ce"),
  new ConfigOptVirtualBool(flagsOpt, CONFIG_CTRL_STATE, CONFIG_CTRL_STATE, "ctrl_state", "cs")
};

ConfigJson config(opts, sizeof(opts) / sizeof(opts[0]), EEPROM_SIZE);

// -------------------------------------------------------------------
// Reset EEPROM, wipes all settings
// -------------------------------------------------------------------
void ResetEEPROM() {
  EEPROM.begin(EEPROM_SIZE);

  //DEBUG.println("Erasing EEPROM");
  for (int i = 0; i < EEPROM_SIZE; ++i) {
    EEPROM.write(i, 0xff);
    //DEBUG.print("#");
  }
  EEPROM.end();
}

// -------------------------------------------------------------------
// Load saved settings from EEPROM
// -------------------------------------------------------------------
void config_load_settings()
{
  config.onChanged(config_changed);

  if(!config.load()) {
    DBUGF("No JSON config found, trying v1 settings");
    config_load_v1_settings();
  }
}

void config_changed(String name)
{
  DBUGF("%s changed", name.c_str());

  if(name.equals(F("flags"))) {
    if(mqtt_connected() != config_mqtt_enabled()) {
      mqtt_restart();
    }
    if(emoncms_connected != config_emoncms_enabled()) {
      emoncms_updated = true;
    } 
  } else if(name.startsWith(F("mqtt_"))) {
    mqtt_restart();
  } else if(name.startsWith(F("emoncms_"))) {
    emoncms_updated = true;
  }
}

void config_commit()
{
  config.commit();
}

bool config_deserialize(String& json) {
  return config.deserialize(json.c_str());
}

bool config_deserialize(const char *json)
{
  return config.deserialize(json);
}

bool config_deserialize(DynamicJsonDocument &doc)
{
  return config.deserialize(doc);
}

bool config_serialize(String& json, bool longNames, bool compactOutput, bool hideSecrets)
{
  return config.serialize(json, longNames, compactOutput, hideSecrets);
}

bool config_serialize(DynamicJsonDocument &doc, bool longNames, bool compactOutput, bool hideSecrets)
{
  return config.serialize(doc, longNames, compactOutput, hideSecrets);
}

void config_set(const char *name, uint32_t val) {
  config.set(name, val);
}
void config_set(const char *name, String val) {
  config.set(name, val);
}
void config_set(const char *name, bool val) {
  config.set(name, val);
}
void config_set(const char *name, double val) {
  config.set(name, val);
}

void config_save_emoncms(bool enable, String server, String path, String node, String apikey,
                    String fingerprint)
{
  uint32_t newflags = flags & ~CONFIG_SERVICE_EMONCMS;
  if(enable) {
    newflags |= CONFIG_SERVICE_EMONCMS;
  }

  config.set(F("emoncms_server"), server);
  config.set(F("emoncms_node"), node);
  config.set(F("emoncms_apikey"), apikey);
  config.set(F("emoncms_fingerprint"), fingerprint);
  config.set(F("flags"), newflags);
  config.commit();
}

void config_save_mqtt(bool enable, String server, int port, String topic, String prefix, String user, String pass)
{
  uint32_t newflags = flags & ~CONFIG_SERVICE_MQTT;
  if(enable) {
    newflags |= CONFIG_SERVICE_MQTT;
  }

  config.set(F("mqtt_server"), server);
  config.set(F("mqtt_port"), port);
  config.set(F("mqtt_topic"), topic);
  config.set(F("mqtt_prefix"), prefix);
  config.set(F("mqtt_user"), user);
  config.set(F("mqtt_pass"), pass);
  config.set(F("flags"), newflags);
  config.commit();
}

void config_save_mqtt_server(String server)
{
  config.set(F("mqtt_server"), server);
  config.commit();
}

void config_save_cal(String voltage, String ct1, String ct2, String freq, String gain
#ifdef SOLAR_METER
  , String svoltage, String sct1, String sct2);
#else
  )
#endif
{
  config.set(F("voltage_cal"), voltage);
  config.set(F("ct1_cal"), ct1);
  config.set(F("ct2_cal"), ct2);
  config.set(F("freq_cal"), freq);
  config.set(F("gain_cal"), gain);
  #ifdef SOLAR_METER
  config.set(F("svoltage_cal"), svoltage);
  config.set(F("sct1_cal"), sct1);
  config.set(F("sct2_cal"), sct2);
  #endif
  config.commit();
}

void config_save_admin(String user, String pass) {
  config.set(F("www_username"), user);
  config.set(F("www_password"), pass);
  config.commit();
}

void config_save_timer(int start1, int stop1, int start2, int stop2, int qvoltage_output, int qtime_offset)
{
  config.set(F("timer_start1"), start1);
  config.set(F("timer_stop1"), stop1);
  config.set(F("timer_start2"), start2);
  config.set(F("timer_stop2"), stop2);
  config.set(F("voltage_output"), qvoltage_output);
  config.set(F("time_offset"), qtime_offset);
  config.commit();
}

void config_save_voltage_output(int qvoltage_output, int save_to_eeprom)
{
  voltage_output = qvoltage_output;

  if (save_to_eeprom) {
    config.set(F("voltage_output"), qvoltage_output);
    config.commit();
  }
}

void config_save_advanced(String hostname) {
  config.set(F("hostname"), hostname);
  config.commit();
}

void config_save_wifi(String qsid, String qpass)
{
  config.set(F("ssid"), qsid);
  config.set(F("pass"), qpass);
  config.commit();
}

void config_save_flags(uint32_t newFlags) {
  config.set(F("flags"), newFlags);
  config.commit();
}

void config_reset() {
  ResetEEPROM();
  config.reset();
}
