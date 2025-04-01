#include <EEPROM.h>

/*
 * 0-17 master password
 * 18-177 16*char[10] service names
 * 178-330 3*pair(char, char[50]) 50 char long passwords
 * 331-561 7*pair(char, char[32]) 32 char long passwords
 * 562-1023 22*pair(char, char[20]) 20 char long passwords
 */
const char* logins[16] = {
  "", // Special case for NULL, leave blank
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  ""
};
const int passwords[32] = {178, 229, 280, 331, 364, 397, 430, 463, 496, 529, 562, 583, 604, 625, 646, 667, 688, 709, 730, 751, 772, 793, 814, 835, 856, 877, 898, 919, 940, 961, 982, 1003};
char memory_buffer[51];

void Serial_write_hex (char number) {
  Serial.print (number >> 4, HEX);
  Serial.print (number & 0x0f, HEX);
}

void Serial_write_dec (char number) {
  Serial.print (number / 10, DEC);
  Serial.print (number % 10, DEC);
}

void Serial_write_fixed (const char* string, char length) {
  Serial.print (string);
  char to_add = length - strlen (string);
  for (char i = 0; i < to_add; i++) {
    Serial.print (' ');
  }
}

void memory_read (char* buffer, int length, int start) { // Outputs length + 1 chars (\0)
  for (int i = 0; i < length; i++) {
    buffer[i] = EEPROM.read (start + i);
  }
  buffer[length] = '\0';
}

void memory_clear () {
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.update(i, '\0');
  }
}

void memory_dump() {
  Serial.println ("Memory dump started");
  for (int i = 0; i < 32; i++) {
    for (int j = 0; j < 32; j++) {
      Serial_write_hex (EEPROM.read (i*32 + j));
      Serial.print(' ');
    }
    Serial.print ('\n');
  }
  Serial.println ("Memory dump finished");
}

char get_password_length (char index) {
  int s_end = (index == 31) ? 1024 : passwords[index + 1];
  return s_end - passwords[index] - 1;
}

void buffer_password_value (char index) {
  memory_read (memory_buffer, get_password_length (index), passwords[index] + 1);
}

void buffer_service_value (char index) {
  memory_read (memory_buffer, 10, 18 + 10 * (EEPROM.read (passwords[index]) >> 4));
}

void buffer_service_name (char index) {
  memory_read (memory_buffer, 10, 10 * index + 18);
}

char get_login_index (char index) {
  return EEPROM.read (passwords[index]) & 0x0f;
}

void password_get (char index) {
  Serial_write_dec (index);
  Serial.print (' ');
  char login_index = get_login_index (index);
  if (login_index != 0) {
    buffer_service_value (index);
    Serial_write_fixed (memory_buffer, 10);
    Serial.print (' ');
    Serial_write_fixed (logins[login_index], 32);
    Serial.print (' ');
    buffer_password_value (index);
    Serial.print (memory_buffer);
  }
  Serial.print ('\n');
}

char Serial_read () {
  while (Serial.available () <= 0) {}
  char ret = Serial.read ();
  if (ret >= 32) {
    return ret;
  } else {
    return Serial_read ();
  }
}

char Serial_read_number () {
  char input = 10 * (Serial_read () - '0');
  return Serial_read () - '0' + input;
}

void memory_set_input (int length, int start) {
  int end = length + start;
  for (; start < end; start++) {
    char char_input = Serial_read ();
    if (char_input == '~') {
      break;
    }
    EEPROM.update (start, char_input);
  }
  EEPROM.update (start, '\0');
}

void passwords_display () {
  for (char i = 0; i < 32; i++) {
    password_get (i);
  }
}

void password_display () {
  Serial.println ("Give password index (like 06, 12, 31)");
  password_get (Serial_read_number ());
}

void password_update () {
  Serial.println ("Give password index (like 06, 12)");
  Serial.println ("Indexes from 00 to 02 are 50 chars, from 03 to 09 are 32 chars and from 10 to 31 are 20 chars");
  passwords_display ();
  char password_index = Serial_read_number ();
  
  Serial.println ("Give service index (like 06, 12)");
  for (char i = 0; i < 16; i++) {
    Serial_write_dec (i);
    Serial.print (' ');
    buffer_service_name (i);
    Serial.println (memory_buffer);
  }
  char service = Serial_read_number ();

  Serial.println ("Give login index (like 06, 12)");
  for (char i = 1; i < 16; i++) {
    Serial_write_dec (i);
    Serial.print (' ');
    Serial.println (logins[i]);
  }
  char login = Serial_read_number ();

  Serial.print ("Give password, ");
  char password_length = get_password_length (password_index);
  Serial.print (password_length, DEC);
  Serial.println (" chars or until '~' is sent");
  memory_set_input (password_length, passwords[password_index] + 1);

  EEPROM.update(passwords[password_index], (service << 4) | (login & 0x0f));

  Serial.println ("Password saved!");
}

void service_update () {
  Serial.println ("Give service index (like 06, 12)");
  for (char i = 0; i < 16; i++) {
    Serial_write_dec (i);
    Serial.print (' ');
    buffer_service_name (i);
    Serial.println (memory_buffer);
  }
  char service = Serial_read_number ();

  Serial.println ("Give service, 10 chars or until '~' is sent");
  memory_set_input (10, service * 10 + 18);

  Serial.println ("Service saved!");
}

void verify_pin () {
  bool is_pin_wrong;
  do {
    is_pin_wrong = false;
    Serial.println ("Type your master password");
    for (char i = 0; i < 18; i++) {
      char char_correct = EEPROM.read (i);
      if (char_correct == '\0') {
        break;
      }
      if (Serial_read () !=  char_correct) {
        is_pin_wrong = true;
      }
    }
  } while (is_pin_wrong);
}

void change_pin () {
  Serial.println ("Update your master password, 18 chars or until '~' is sent");
  memory_set_input (18, 0);
  Serial.println ("Password updated");
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(60000);
  verify_pin ();
}

void loop() {
  Serial.println ("r to read passwords, R to read single password, w to update password, c to clear everything, s for service update, p for master password update");
  int input = Serial_read ();
  switch (input) {
    case 'w':
      password_update ();
      break;
    case 'r':
      passwords_display ();
      break;
    case 'R':
      password_display ();
      break;
    case 'c':
      memory_clear ();
      break;
    case 's':
      service_update ();
      break;
    case 'p':
      change_pin ();
      break;
  }
}
