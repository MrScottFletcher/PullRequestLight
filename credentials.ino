#include <EEPROM.h>

// Read parameters from EEPROM or Serial
void readCredentials()
{

    //Stack up the addresses based on the prior allocated spaces
    int ssidAddr = 0;
    int passAddr = ssidAddr + SSID_LEN;
    int ado_connectionStringAddr = passAddr + PASS_LEN;
    int ado_apiaccesstokenStringAddr = ado_connectionStringAddr + ADO_CONNECTION_STRING_LEN;
    int iot_connectionStringAddr = ado_apiaccesstokenStringAddr + ADO_API_KEY_LEN;

    // malloc for parameters
    ssid = (char *)malloc(SSID_LEN);
    pass = (char *)malloc(PASS_LEN);
    ado_connectionString = (char *)malloc(ADO_CONNECTION_STRING_LEN);
    ado_apiaccesstokenString = (char*)malloc(ADO_API_KEY_LEN);
    //iot_connectionString = (char*)malloc(IOT_CONNECTION_STRING_LEN);

    // try to read out the credential information, if failed, the length should be 0.
    int ssidLength = EEPROMread(ssidAddr, ssid);
    int passLength = EEPROMread(passAddr, pass);
    int ado_connectionStringLength = EEPROMread(ado_connectionStringAddr, ado_connectionString);
    int ado_apiaccesstokenStringLength = EEPROMread(ado_apiaccesstokenStringAddr, ado_apiaccesstokenString);
    //int iot_connectionStringLength = EEPROMread(iot_connectionStringAddr, iot_connectionString);

    if (ssidLength > 0 && passLength > 0 && ado_connectionStringLength > 0 && !needEraseEEPROM())
    {
        return;
    }

    // read from Serial and save to EEPROM
    readFromSerial("Input your Wi-Fi SSID: ", ssid, SSID_LEN, 0);
    EEPROMWrite(ssidAddr, ssid, strlen(ssid));

    readFromSerial("Input your Wi-Fi password: ", pass, PASS_LEN, 0);
    EEPROMWrite(passAddr, pass, strlen(pass));

    readFromSerial("Input your Azure DevOps PullRequest path string: ", ado_connectionString, ADO_CONNECTION_STRING_LEN, 0);
    EEPROMWrite(ado_connectionStringAddr, ado_connectionString, strlen(ado_connectionString));

    readFromSerial("Input your Azure DevOps API Access Token string: ", ado_apiaccesstokenString, ADO_API_KEY_LEN, 0);
    EEPROMWrite(ado_apiaccesstokenStringAddr, ado_apiaccesstokenString, strlen(ado_apiaccesstokenString));

    //readFromSerial("Input your Azure IoT hub device connection string: ", iot_connectionString, IOT_CONNECTION_STRING_LEN, 0);
    //EEPROMWrite(iot_connectionStringAddr, iot_connectionString, strlen(iot_connectionString));
}

bool needEraseEEPROM()
{
    char result = 'n';
    readFromSerial("Do you need re-input your credential information?(Auto skip this after 10 seconds)[Y/n]", &result, 1, 10000);
    if (result == 'Y' || result == 'y')
    {
        clearParam();
        return true;
    }
    return false;
}

// reset the EEPROM
void clearParam()
{
    char data[EEPROM_SIZE];
    memset(data, '\0', EEPROM_SIZE);
    EEPROMWrite(0, data, EEPROM_SIZE);
}

#define EEPROM_END 0
#define EEPROM_START 1
void EEPROMWrite(int addr, char *data, int size)
{
    EEPROM.begin(EEPROM_SIZE);
    // write the start marker
    EEPROM.write(addr, EEPROM_START);
    addr++;
    for (int i = 0; i < size; i++)
    {
        EEPROM.write(addr, data[i]);
        addr++;
    }
    EEPROM.write(addr, EEPROM_END);
    EEPROM.commit();
    EEPROM.end();
}

// read bytes from addr util '\0'
// return the length of read out.
int EEPROMread(int addr, char *buf)
{
    EEPROM.begin(EEPROM_SIZE);
    int count = -1;
    char c = EEPROM.read(addr);
    addr++;
    if (c != EEPROM_START)
    {
        return 0;
    }
    while (c != EEPROM_END && count < EEPROM_SIZE)
    {
        c = (char)EEPROM.read(addr);
        count++;
        addr++;
        buf[count] = c;
    }
    EEPROM.end();
    return count;
}
