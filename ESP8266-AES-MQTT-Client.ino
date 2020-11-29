#include "AESLib.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

AESLib aesLib;

const char* ssid = "ssid";
const char* password = "password";
const char* mqtt_server = "raspeberrypi";
const char* mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

int time_count = 20;

char cleartext[256];
char ciphertext[512];
String encrypted, decrypted;

// AES Encryption Key
byte aes_key[] = { 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 };

// General initialization vector
byte aes_iv[N_BLOCK] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Generate IV (once)
void aes_init() {
  aesLib.gen_iv(aes_iv);
}

String encrypt(char * msg, uint16_t msgLen, byte iv[]) {

  int cipherlength = aesLib.get_cipher_length(msgLen);
  char encrypted[cipherlength]; // AHA! needs to be large, 2x is not enough
  aesLib.encrypt64(msg, msgLen, encrypted, aes_key, sizeof(aes_key), iv);
  return String(encrypted);
}

String decrypt(char * msg, uint16_t msgLen, byte iv[]) {

  char decrypted[msgLen];
  aesLib.decrypt64(msg, msgLen, decrypted, aes_key, sizeof(aes_key), iv);
  return String(decrypted);
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

 }

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial); // wait for serial port
  delay(2000);
  aes_init();
  aesLib.set_paddingmode(paddingMode::Array);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  char b64in[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  char b64out[base64_enc_len(sizeof(aes_iv))];
  base64_encode(b64out, b64in, 16);

  char b64enc[base64_enc_len(10)];
  base64_encode(b64enc, (char*) "0123456789", 10);

  char b64dec[ base64_dec_len(b64enc, sizeof(b64enc))];
  base64_decode(b64dec, b64enc, sizeof(b64enc));

}

/* non-blocking wait function */
void wait(unsigned long milliseconds) {
  unsigned long timeout = millis() + milliseconds;
  while (millis() < timeout) {
    yield();
  }
}

unsigned long loopcount = 0;
unsigned long time_sum = 0;
unsigned long time_sum_enc = 0;
unsigned long time_sum_dec = 0;

byte enc_iv[N_BLOCK] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // iv_block gets written to, provide own fresh copy...
byte dec_iv[N_BLOCK] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void MQTT_Publish (String encrypted, String decrypted) {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  snprintf (msg, MSG_BUFFER_SIZE, encrypted.c_str());
  Serial.print("Publish message encrypted "); Serial.println(msg);
  client.publish("MACC/IoT/Esp8266/Encrypted", msg);

  snprintf (msg, MSG_BUFFER_SIZE, decrypted.c_str());
  Serial.print("Publish message decrypted: "); Serial.println(msg);
  client.publish("MACC/IoT/Esp8266/Decrypted", msg);

}

void loop() {

unsigned long lastSend;
  
  String readBuffer = String(random(50));
 
  Serial.println("INPUT:" + readBuffer);
  
  loopcount = 0;

  for (int i = 0; i < time_count; i++) {

    loopcount++; Serial.println(loopcount); // entry counter

    sprintf(cleartext, "%s", readBuffer.c_str()); // must not exceed 255 bytes; may contain a newline

    // Encrypt
    unsigned long time_start_enc = micros();
    uint16_t clen = String(cleartext).length();
    encrypted = encrypt(cleartext, clen, enc_iv);
    unsigned long time_aux_enc = micros() - time_start_enc;
    time_sum_enc = time_sum_enc + time_aux_enc;
    Serial.println("ENCRYPT");
    sprintf(ciphertext, "%s", encrypted.c_str());
    //Serial.print("Ciphertext: "); Serial.println(encrypted);
    Serial.print("Encryption time: "); Serial.print(time_aux_enc); Serial.println("us");
    delay(500);

    // Decrypt
    unsigned long time_start_dec = micros();
    uint16_t dlen = encrypted.length();
    decrypted = decrypt(ciphertext, dlen, dec_iv);
    unsigned long time_aux_dec = micros() - time_start_dec;
    time_sum_dec = time_sum_dec + time_aux_dec;
    Serial.println("DECRYPT");
    //Serial.print("Cleartext: "); Serial.println(decrypted);
    Serial.print("Decryption time: "); Serial.print(time_aux_dec); Serial.println("us");
    delay(500);

    // Validation
    if (decrypted.equals(cleartext)) {
      Serial.println("SUCCES");
    }
    else
    {
      Serial.println("FAILURE");

    }
    for (int i = 0; i < 16; i++) {
      enc_iv[i] = 0;
      dec_iv[i] = 0;
    }
  }
  unsigned long time_avg_enc = time_sum_enc / time_count;
  unsigned long time_avg_dec = time_sum_dec / time_count;
  Serial.println("--------------------------");
  Serial.print("Encryption time average: "); Serial.print(time_avg_enc); Serial.println("us");
  Serial.print("Decryption time average: "); Serial.print(time_avg_dec); Serial.println("us");
  Serial.println("--------------------------");

  MQTT_Publish (encrypted, decrypted);
 }
