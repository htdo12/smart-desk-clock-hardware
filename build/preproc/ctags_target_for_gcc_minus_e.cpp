# 1 "/Users/huit/Arduino Project/smart_desk_clock/sdclock.ino"
# 2 "/Users/huit/Arduino Project/smart_desk_clock/sdclock.ino" 2
# 3 "/Users/huit/Arduino Project/smart_desk_clock/sdclock.ino" 2
# 4 "/Users/huit/Arduino Project/smart_desk_clock/sdclock.ino" 2
# 5 "/Users/huit/Arduino Project/smart_desk_clock/sdclock.ino" 2
# 6 "/Users/huit/Arduino Project/smart_desk_clock/sdclock.ino" 2
# 7 "/Users/huit/Arduino Project/smart_desk_clock/sdclock.ino" 2
# 8 "/Users/huit/Arduino Project/smart_desk_clock/sdclock.ino" 2
# 9 "/Users/huit/Arduino Project/smart_desk_clock/sdclock.ino" 2
# 10 "/Users/huit/Arduino Project/smart_desk_clock/sdclock.ino" 2

// Provide the token generation process info.
# 13 "/Users/huit/Arduino Project/smart_desk_clock/sdclock.ino" 2

// Provide the RTDB payload printing info and other helper functions.
# 16 "/Users/huit/Arduino Project/smart_desk_clock/sdclock.ino" 2

/* 1. Define the WiFi credentials */
// #define WIFI_SSID "E.504"
// #define WIFI_PASSWORD "12345@12345"
// #define WIFI_SSID "PhongMayTinh"
// #define WIFI_PASSWORD "ttcnttsgu"
// #define WIFI_SSID "SWEBI COFFEE 1"
// #define WIFI_PASSWORD "250tenlua"
// #define WIFI_SSID "Phong 6.6_2.4G"
// #define WIFI_PASSWORD "quahoianhkhang"





// For the following credentials, see examples/Authentications/SignInAsUser/EmailPassword/EmailPassword.ino

/* 2. Define the API Key */


/* 3. Define the RTDB URL */


/* 4. Define the user Email and password that alreadey registerd or added in your project */

// Define Firebase Data object
FirebaseData stream;
FirebaseData fdbo;
FirebaseAuth auth;
FirebaseConfig config;



DHT dht(D7 /* Digital pin connected to the DHT sensor*/, DHT22);



const long utcOffsetInSeconds = 25200;
unsigned long circleBlink = 0;
unsigned long syncTemp = 0;
unsigned long delayRealTimeTemperature = 0;
unsigned long timeRepeat = 0;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", utcOffsetInSeconds);
DS3232RTC myRTC;
TM1637Display display(D5, D6);
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4);

unsigned long sendDataPrevMillis = 0;

String parentPath = "/get/";
String childPath[3] = {"alarmtime", "lamp", "sleepingtime"};
volatile bool dataChanged = false;
bool isalarmtimeon = false;
bool islampon = false;
bool issleepingtimeon = false;
time_t alarmtime = 1669957393;
int lightslider = 0;
time_t startsleepingtime = 1669957393;
time_t endsleepingtime = 1669957393;
bool setBUZZER = false;

bool convertStringToBool(String value)
{
  return value == "true";
}
void streamCallback(MultiPathStream stream)
{
  Serial.println("data change");
  size_t numChild = sizeof(childPath) / sizeof(childPath[0]);

  for (size_t i = 0; i < numChild; i++)
  {
    if (stream.get(childPath[i]))
    {
      Serial.printf("path: %s, event: %s, type: %s, value: %s%s", stream.dataPath.c_str(), stream.eventType.c_str(), stream.type.c_str(), stream.value.c_str(), i < numChild - 1 ? "\n" : "");
      if (stream.dataPath == "/alarmtime/isset")
      {
        isalarmtimeon = convertStringToBool(stream.value);
        continue;
      }
      if (stream.dataPath == "/alarmtime/time")
      {
        alarmtime = (long)stream.value.toDouble();
        continue;
      }
      if (stream.dataPath == "/lamp/islight")
      {
        islampon = convertStringToBool(stream.value);
        continue;
      }
      if (stream.dataPath == "/lamp/lightbrightness")
      {
        lightslider = stream.value.toInt();
        continue;
      }
      if (stream.dataPath == "/sleepingtime/isset")
      {
        issleepingtimeon = convertStringToBool(stream.value);
        continue;
      }
      if (stream.dataPath == "/sleepingtime/starttime")
      {
        startsleepingtime = (long)stream.value.toDouble();
        continue;
      }
      if (stream.dataPath == "/sleepingtime/endtime")
      {
        endsleepingtime = (long)stream.value.toDouble();
        continue;
      }
    }
  }

  Serial.println();

  // This is the size of stream payload received (current and max value)
  // Max payload size is the payload size under the stream path since the stream connected
  // and read once and will not update until stream reconnection takes place.
  // This max value will be zero as no payload received in case of ESP8266 which
  // BearSSL reserved Rx buffer size is less than the actual stream payload.
  Serial.printf("Received stream payload size: %d (Max. %d)\n\n", stream.payloadLength(), stream.maxPayloadLength());

  // Due to limited of stack memory, do not perform any task that used large memory here especially starting connect to server.
  // Just set this flag and check it status later.
  dataChanged = true;
}

void streamTimeoutCallback(bool timeout)
{
  if (timeout)
    Serial.println("stream timed out, resuming...\n");

  if (!stream.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
}
bool isSleepingTimeON()
{
  return issleepingtimeon;
}
bool isAlarmTimeon()
{
  return isalarmtimeon;
}
bool isLampOn()
{
  return islampon;
}
time_t getAlarmTime()
{
  return alarmtime;
}
time_t getStartSleepingTime()
{
  return startsleepingtime;
}
time_t getEndSleepingTime()
{
  return endsleepingtime;
}
int getLampValue()
{
  return lightslider;
}
void syncDataFirebase()
{
  if (Firebase.ready())
  {
    if (Firebase.RTDB.getDouble(&fdbo, ("/get/alarmtime/time")))
    {
      alarmtime = (long)fdbo.to<double>();
    }
    if (Firebase.RTDB.getDouble(&fdbo, ("/get/sleepingtime/starttime")))
    {
      startsleepingtime = (long)fdbo.to<double>();
    }
    if (Firebase.RTDB.getDouble(&fdbo, ("/get/sleepingtime/endtime")))
    {
      endsleepingtime = (long)fdbo.to<double>();
    }
    if (Firebase.RTDB.getBool(&fdbo, ("/get/alarmtime/isset")))
    {
      isalarmtimeon = fdbo.to<bool>();
    }
    if (Firebase.RTDB.getBool(&fdbo, ("/get/sleepingtime/isset")))
    {
      issleepingtimeon = fdbo.to<bool>();
    }
    if (Firebase.RTDB.getBool(&fdbo, ("/get/lamp/islight")))
    {
      islampon = fdbo.to<bool>();
    }
    if (Firebase.RTDB.getInt(&fdbo, ("/get/lamp/lightbrightness")))
    {
      lightslider = fdbo.to<int>();
    }
  }
  Serial.println(alarmtime);
  Serial.println(startsleepingtime);
  Serial.println(endsleepingtime);
  Serial.println(isalarmtimeon);
  Serial.println(issleepingtimeon);
  Serial.println(islampon);
}
void setAlarmOff(bool isset)
{
  if (!Firebase.RTDB.setBoolAsync(&fdbo, "/get/alarmtime/isset", isset))
  {
    Serial.println("setAlarmOff error");
  }
}
void setSleepingTimeOff(bool isset)
{
  if (!Firebase.RTDB.setBoolAsync(&fdbo, "/get/sleepingtime/isset", isset))
  {
    Serial.println("setSleepingTimeOff error");
  }
}
void setIsLightforSleeping(bool isset)
{
  if (!Firebase.RTDB.setBoolAsync(&fdbo, "/get/lamp/islight", isset))
  {
    Serial.println("setIsLightforSleeping error");
  }
}
void setRealTimeTemperature(float temperature)
{
  if (!Firebase.RTDB.setDoubleAsync(&fdbo, "/set/temperature", temperature))
  {
    Serial.println("setRealTimeTemperature error");
  }
}
void setRealTimeHumidity(float humidity)
{
  if (!Firebase.RTDB.setDoubleAsync(&fdbo, "/set/humidity", humidity))
  {
    Serial.println("setRealTimeHumidity error");
  }
}
void addRecordTemperature(time_t timeStamp, float temperature)
{
  char path[50];
  sprintf(path, "/set/temperatures/%lu", timeStamp);
  if (!Firebase.RTDB.setFloatAsync(&fdbo, path, temperature))
    ;
}
void setSlider(int value)
{
  char path[50];
  sprintf(path, "/get/lamp/lightbrightness", value);
  if (!Firebase.RTDB.setIntAsync(&fdbo, path, value))
  {
    Serial.println("SetSlider error");
  }
}
void setup()
{

  Serial.begin(115200);

  WiFi.begin("Huy Thong", "0978829111");
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", "4.2.7");

  /* Assign the api key (required) */
  //   config.api_key = API_KEY;

  //   /* Assign the user sign in credentials */
  //   auth.user.email = USER_EMAIL;
  //   auth.user.password = USER_PASSWORD;

  //   /* Assign the RTDB URL (required) */
  //   config.database_url = DATABASE_URL;

  //   /* Assign the callback function for the long running token generation task */
  // config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Or use legacy authenticate method
  config.database_url = "iot-smart-clock-d6129-default-rtdb.firebaseio.com" /*<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app*/;
  config.signer.tokens.legacy_token = "I8o4vZpREO9hstS5GGzChpGVFumdctRjsbu4391u";

  // To connect without auth in Test Mode, see Authentications/TestMode/TestMode.ino

  Firebase.begin(&config, &auth);

  Firebase.reconnectWiFi(true);

  // Recommend for ESP8266 stream, adjust the buffer size to match your stream data size

  stream.setBSSLBufferSize(2048 /* Rx in bytes, 512 - 16384 */, 512 /* Tx in bytes, 512 - 16384 */);

  // The data under the node being stream (parent path) should keep small
  // Large stream payload leads to the parsing error due to memory allocation.

  // The MultiPathStream works as normal stream with the payload parsing function.

  if (!Firebase.RTDB.beginMultiPathStream(&stream, parentPath))
    Serial.printf("sream begin error, %s\n\n", stream.errorReason().c_str());

  Firebase.RTDB.setMultiPathStreamCallback(&stream, streamCallback, streamTimeoutCallback);

  /** Timeout options, below is default config.



    //WiFi reconnect timeout (interval) in ms (10 sec - 5 min) when WiFi disconnected.

    config.timeout.wifiReconnect = 10 * 1000;



    //Socket begin connection timeout (ESP32) or data transfer timeout (ESP8266) in ms (1 sec - 1 min).

    config.timeout.socketConnection = 30 * 1000;



    //ESP32 SSL handshake in ms (1 sec - 2 min). This option doesn't allow in ESP8266 core library.

    config.timeout.sslHandshake = 2 * 60 * 1000;



    //Server response read timeout in ms (1 sec - 1 min).

    config.timeout.serverResponse = 10 * 1000;



    //RTDB Stream keep-alive timeout in ms (20 sec - 2 min) when no server's keep-alive event data received.

    config.timeout.rtdbKeepAlive = 45 * 1000;



    //RTDB Stream reconnect timeout (interval) in ms (1 sec - 1 min) when RTDB Stream closed and want to resume.

    config.timeout.rtdbStreamReconnect = 1 * 1000;



    //RTDB Stream error notification timeout (interval) in ms (3 sec - 30 sec). It determines how often the readStream

    //will return false (error) when it called repeatedly in loop.

    config.timeout.rtdbStreamError = 3 * 1000;



  */
# 353 "/Users/huit/Arduino Project/smart_desk_clock/sdclock.ino"
  syncDataFirebase();
  pinMode(D4, 0x01);
  pinMode(D8, 0x01);
  pinMode(D3, 0x02);
  dht.begin();
  Wire.begin();
  myRTC.begin();
  display.setBrightness(5);
  lcd.begin();
  lcd.setCursor(2, 0);
  lcd.print("Smart Desk Clock");
  lcd.setCursor(2, 1);
  lcd.print("Nguyen Duy Thanh");
  lcd.setCursor(4, 2);
  lcd.print("Do Huy Thong");
  delay(3000);
  lcd.clear();
  setTime();
}

void loop()
{
  lampController();
  showTemp();
  showTimeFrommyRTC();
  setTimeSleeping();
  setAlarm();
}

void lampController()
{
  if (isLampOn())
  {

    Serial.println("lamp on");
    int value = getLampValue();
    Serial.println(value);
    analogWrite(D4, value);
  }
  else
  {
    Serial.println("lamp off");
    analogWrite(D4, 0);
  }
}

void showTemp()
{
  time_t stampTime = now();
  // Create °C symbol
  const uint8_t celsius_circle[] = {
      0b00000001 | 0b00000010 | 0b00100000 | 0b01000000, // Circle
      0b00000001 | 0b00001000 | 0b00010000 | 0b00100000 // C
  };
  // Create C symbol
  const uint8_t celsius_noCircle[] = {
      0x00 | 0x00 | 0x00 | 0x00, // no circle
      0b00000001 | 0b00001000 | 0b00010000 | 0b00100000 // C
  };

  float temperature_celsius = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (millis() - delayRealTimeTemperature >= 1000)
  {
    setRealTimeTemperature(temperature_celsius);
    setRealTimeHumidity(humidity);
    delayRealTimeTemperature = millis();
  }
  // Display the temperature in celsius format
  display.showNumberDec(temperature_celsius, false, 2, 0);

  // Show ° Symbol blink each 500ms
  if (millis() - circleBlink >= 500)
  {
    display.setSegments(celsius_circle, 2, 2);
  }

  // Hide ° Symbol blink each 1000ms
  if (millis() - circleBlink >= 1000)
  {
    display.setSegments(celsius_noCircle, 2, 2);
    circleBlink = millis();
  }

  // Add new record Tempperture each 1 seconds
  if (millis() - syncTemp >= 1000)
  {
    addRecordTemperature(stampTime, temperature_celsius);
    syncTemp = millis();
  }
}

void setTime()
{
  while (timeClient.update())
    ;
  time_t epochTime = timeClient.getEpochTime();
  /*

  Serial.println(epochTime);

  int currentHour = timeClient.getHours();

  Serial.print("Hour: ");

  Serial.println(currentHour);

  int currentMinute = timeClient.getMinutes();

  Serial.print("Minutes: ");

  Serial.println(currentMinute);

  int currentSecond = timeClient.getSeconds();

  Serial.print("Seconds: ");

  Serial.println(currentSecond);

  struct tm *ptm = gmtime((time_t *)&epochTime);



  int monthDay = ptm->tm_mday;

  Serial.print("Month day: ");

  Serial.println(monthDay);

  int currentMonth = ptm->tm_mon + 1;

  Serial.print("Month: ");

  Serial.println(currentMonth);

  int currentYear = ptm->tm_year + 1900;

  Serial.print("Year: ");

  Serial.println(currentYear);

  */
# 474 "/Users/huit/Arduino Project/smart_desk_clock/sdclock.ino"
  myRTC.set(epochTime);
  setTime(epochTime);
}

void showTimeFrommyRTC()
{
  time_t stampTime = now();

  lcd.setCursor(0, 0);
  lcd.print(dayStr(weekday(stampTime)));
  lcd.setCursor(10, 0);
  lcd.print(day(stampTime), 10);
  lcd.print("/");
  lcd.print(month(stampTime), 10);
  lcd.print("/");
  lcd.print(year(stampTime), 10);

  lcd.setCursor(3, 1);
  lcd.print("Time: ");
  lcd.setCursor(9, 1);
  if (hour(stampTime) <= 9)
  {
    lcd.print("0");
    lcd.print(hour(stampTime), 10);
  }
  else
  {
    lcd.print(hour(stampTime), 10);
  }
  lcd.print(":");
  if (minute(stampTime) <= 9)
  {
    lcd.print("0");
    lcd.print(minute(stampTime), 10);
  }
  else
  {
    lcd.print(minute(stampTime), 10);
  }
  lcd.print(":");
  if (second(stampTime) <= 9)
  {
    lcd.print("0");
    lcd.print(second(stampTime), 10);
  }
  else
  {
    lcd.print(second(stampTime), 10);
  }

  if (isAlarmTimeon())
  {
    lcd.setCursor(0, 2);
    lcd.print("Alarm:On ");
  }
  else
  {
    lcd.setCursor(0, 2);
    lcd.print("Alarm:Off");
  }

  if (isSleepingTimeON())
  {
    lcd.setCursor(10, 2);
    lcd.print("Sleep:On ");
  }
  else
  {
    lcd.setCursor(10, 2);
    lcd.print("Sleep:Off");
  }

  float humidity = dht.readHumidity();
  lcd.setCursor(0, 3);
  lcd.print("Humidity: ");
  lcd.setCursor(10, 3);
  lcd.print(humidity);
  lcd.setCursor(15, 3);
  lcd.print("%");

  /*

    Serial.println(hour(t));

    Serial.println(minute(t));

    Serial.println(second(t));

  */
# 559 "/Users/huit/Arduino Project/smart_desk_clock/sdclock.ino"
}

void setTimeSleeping()
{
  time_t timeStart = getStartSleepingTime();
  time_t timeEnd = getEndSleepingTime();
  time_t timeNow = now();

  if (isSleepingTimeON())
  {
    if (timeStart - timeNow == 0)
    {
      setIsLightforSleeping(true);
      setSlider(255);
    }
    if (timeNow == timeStart + 60)
    {
      setIsLightforSleeping(true);
      setSlider(230);
    }
    if (timeNow == timeStart + 120)
    {
      setIsLightforSleeping(true);
      setSlider(205);
    }
    if (timeNow == timeStart + 180)
    {
      setIsLightforSleeping(true);
      setSlider(180);
    }
    if (timeNow == timeStart + 240)
    {
      setIsLightforSleeping(true);
      setSlider(165);
    }
    if (timeNow == timeStart + 300)
    {
      setIsLightforSleeping(true);
      setSlider(140);
    }
    if (timeNow == timeStart + 360)
    {
      setIsLightforSleeping(true);
      setSlider(115);
    }
    if (timeNow == timeStart + 420)
    {
      setIsLightforSleeping(true);
      setSlider(90);
    }
    if (timeNow == timeStart + 480)
    {
      setIsLightforSleeping(true);
      setSlider(65);
    }
    if (timeNow == timeStart + 540)
    {
      setIsLightforSleeping(true);
      setSlider(40);
    }
    if (timeNow >= timeStart + 600)
    {
      setIsLightforSleeping(true);
      setSlider(15);
    }
    if (timeEnd - timeNow == 0)
    {
      setIsLightforSleeping(false);
      setSleepingTimeOff(false);
      setSlider(0);
    }

    Serial.println(timeNow);
    Serial.println(timeStart);
    Serial.println(timeEnd);
  }
  if (!isSleepingTimeON())
  {
    setIsLightforSleeping(false);
    setSlider(0);
  }
}

void setAlarm()
{
  time_t timeSet = getAlarmTime();
  time_t timeNow = now();

  if (isAlarmTimeon())
  {
    if (timeSet - timeNow == 0)
    {
      setBUZZER = true;
    }
  }
  else
  {
    setBUZZER = false;
  }

  if (digitalRead(D3) == 0)
  {
    setAlarmOff(false);
  }

  if (setBUZZER)
  {
    if (millis() - timeRepeat <= 100)
    {
      tone(D8, 20);
    }
    if (millis() - timeRepeat <= 200)
    {
      tone(D8, 20);
    }
    if (millis() - timeRepeat > 600)
    {
      tone(D8, 0);
    }
    if (millis() - timeRepeat >= 1000)
    {
      timeRepeat = millis();
    }
  }
  else
  {
    tone(D8, 0);
  }
}
