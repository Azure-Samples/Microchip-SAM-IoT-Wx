# Using the SAM-IoT as a Cloud Agent for an Application Host Processor

## Introduction

This document describes how to utilize the SAM-IoT WG Development Board as a cloud agent – providing a serial-to-cloud bridge between an application processor (e.g. PIC-IoT Wx Development Board) and an IoT Central application. The SAM-IoT WG Development Board will be provisioned for use with Azure IoT services using self-signed X.509 certificate-based authentication.



## Table of Contents

- [Introduction](#introduction)
- [Background Knowledge](#background-knowledge)
  - [Application Processor Connections to the SAM-IoT Cloud Agent](#Application-Processor-Connections-to-the-SAM-IoT-Cloud-Agent)
  - [Sending Application Processor Telemetry Events](#Sending-Application-Processor-Telemetry-Events)
  - [Reading/Writing Application Processor Properties](#Reading/Writing-Application-Processor-Properties)
  - [Dedicated Telemetry Interface (DTI)](#Dedicated-Telemetry-Interface-(DTI))
  - [Data Frame Format](#Data-Frame-Format)
- [Program the DTI Test Platform using a PIC-IoT Development Board Configured as the SPI Master](#Program-the-DTI-Test-Platform-using-a-PIC-IoT-Development-Board-Configured-as-the-SPI-Master)
  - [Installing the Development Tools](#installing-the-development-tools)
  - [Program SAM-IoT Development Board Configured as SPI Slave](#Program-SAM-IoT-Development-Board-Configured-as-SPI-Slave)
  - [Create an IoT Central Application for your Device](#create-an-iot-central-application-for-your-device)
  - [Validate connection](#validate-connection)
- [Data Visualization using the IoT Central Application](#Data-visualization-using-the-IoT-Central-Application)
  - [Confirmation of Reception for Every Data Frame Sent by the Application Processor](#Confirmation-of-Reception-for-Every-Data-Frame-Sent-by-the-Application-processor)

    

## Background Knowledge

### Application Processor Connections to the SAM-IoT Cloud Agent

A total of 5 wires need to be connected between the application host processor and the SAM-IoT development board. The SAM-IoT board basically acts as a proxy/bridge between an application processor and the Cloud.  The Command Line Interface (CLI) requires 2 wires for a UART connection and the Dedicated Telemetry Interface (DTI) requires 3 wires for a uni-directional SPI Master-to-Slave connection.  The SPI pins can be easily accessed using the J20 header of the SAM-IoT board (unfortunately the UART pins are not broken out to either of the 2 headers, so refer to the [SAM-IoT board schematic](https://www.microchip.com/content/dam/mchp/documents/MCU32/ProductDocuments/UserGuides/SAM-IoT-WG-Development-Board-User-Guide.pdf) for connecting directly to the PB02 & PB03 pins).

<img src=".//media/image119.png"/>



### Sending Application Processor Telemetry Events

The `telemetry` command is executed using the CLI to update any of the available telemetry variables by using a pre-defined index to select the desired one

```bash
>telemetry <index>,<data>
```
NOTE: The number of characters for the CLI command cannot exceed a total of 80 characters

<img src=".//media/image120.png"/>



### Reading/Writing Application Processor Properties

The `property` command is executed using the CLI to update (write to) any of the available properties by using a pre-defined index to select the desired one

```bash
>property <index>,<data>
```
NOTE: The number of characters for the CLI command cannot exceed a total of 80 characters

<img src=".//media/image121.png"/>



### Dedicated Telemetry Interface (DTI)

The DTI is implemented on the SAM-IoT configured as a SPI Slave. The DTI should be used when larger telemetry chunks of data need to be sent which cannot be executed using the CLI (due to the limitation of the maximum number of characters that the CLI can process for a single command line).  The external application processor must be configured for SPI Master mode and be connected to SAM-IoT board's J-20 header using 3 wires (MOSI, SCK, CS).  The SPI Master must send a valid data frame in the correct format (header + payload) for every telemetry update operation.

For the most reliable performance, the SPI Master should adhere to the following operating parameters:

- Interframe delay = 500 msec minimum
- SCK = 30 kHz maximum
- [Recommendation] Issue the `telemetry 0,0` command on the CLI prior to sending an individual data frame (or right before the start of a group of consecutive data frames).  This command will ensure that the DTI receive buffer has been properly reset prior to receiving the first byte of the incoming data frame sent by the SPI Master

<img src=".//media/image122.png"/>

### Data Frame Format

Each SPI transaction must follow the format for sending out a valid data frame through the DTI.  A valid data frame must start with a 4-byte header followed by the payload data.  The command byte must be equal to the hex value corresponding to ASCII character 'T' or 't' (signifying that the data frame is to be processed as a `telemetry` command)

<img src=".//media/image123.png"/>

For example, the following data frame tells the SAM-IoT to update the `telemetry_Str_1` value in the Cloud with a 1KB array of bytes that begins with "START", terminated with "END", and every byte in between with consecutive '1's

<img src=".//media/image124.png"/>



## Program the DTI Test Platform using a PIC-IoT Development Board Configured as the SPI Master

1. Clone/download the SPI code example project by issuing the following commands in a `Command Prompt` or `PowerShell` window

   ```bash
   git clone https://github.com/randywu763/MCHP-IoT_SPI.git
   cd MCHP-IoT_SPI
   git submodule update --init
   ```

2. Connect the **PIC-IoT Development Board** to the PC, then make sure `CURIOSITY` device shows up as a disk drive on the `Desktop` or in a `File Explorer` window. Drag and drop (i.e. copy) the pre-built `*.hex` file (located in the folder at `MCHIP-IoT_SPI` > `PIC-IoT_SPI-Master.X` > `dist` > `PIC24_IOT_WG` > `production`) to the `CURIOSITY` drive.

   <img src=".//media/image130.png"/>



## Installing the Development Tools

Embedded software development tools from Microchip need to be pre-installed in order to properly program the SAM-IoT WG Development Board and provision it for use with Microsoft Azure IoT services.

Click this link for the setup procedure: [Development Tools Installation](https://github.com/Azure-Samples/Microchip-SAM-IoT-Wx/blob/main/Dev_Tools_Install.md)



## Program SAM-IoT Development Board Configured as SPI Slave

1. Clone/download the MPLAB X demo project by issuing the following commands in a `Command Prompt` or `PowerShell` window.

   ```bash
   git clone https://github.com/Azure-Samples/Microchip-SAM-IoT-Wx.git
   cd Microchip-SAM-IoT-Wx
   git submodule update --init
   ```

2. Make the appropriate jumper connections between the J20 headers on both development boards.

   <img src=".//media/image125.png"/>

   

3. Connect the **SAM-IoT Development Board** to the PC, then make sure `CURIOSITY` device shows up as a disk drive on the `Desktop` or in a `File Explorer` window. 

4. Connect the board to PC, then make sure `CURIOSITY` device shows up as a disk drive on the `Desktop` or in a `File Explorer` window. Drag and drop (i.e. copy) the pre-built `*.hex` file (located in the folder at `Microchip-SAM-IoT-Wx` > `firmware` > `AzurePnPDps.X` > `dist` > `SAMD21_WG_IOT` > `production`) to the `CURIOSITY` drive

   [![img](.//media/image115.png)](https://github.com/Azure-Samples/Microchip-SAM-IoT-Wx/blob/main/media/image115.png)

5. Set up a Command Line Interface (CLI) to the board.

- Open a serial terminal (e.g. PuTTY, TeraTerm, etc.) and connect to the COM port corresponding to the **SAM-IoT Development Board** at `9600 baud` (e.g. open PuTTY Configuration window &gt; choose `session` &gt; choose `Serial`&gt; Enter the right COMx port). You can find the COM info by opening your PC’s `Device Manager` &gt; expand `Ports(COM & LPT)` &gt; take note of `Curiosity Virtual COM Port` .

<img src=".//media/image27.png">

4. Before typing anything in the terminal emulator window, **disable** the local echo feature in the terminal settings for best results.  In the terminal window, hit `[RETURN]` to bring up the Command Line Interface prompt (which is simply the `>` character). Type `help` and then hit `[RETURN]` to get the list of available commands for the CLI.  The Command Line Interface allows you to send simple ASCII-string commands to set or get the user-configurable operating parameters of the application while it is running.

<img src=".//media/image44.png" style="width:5.in;height:3.18982in"/>

5. In the terminal emulator window, set the debug messaging level to 0 to temporarily disable the output messages. The debug level can be set anywhere from 0 to 4.  Use the `debug <level>` command by manually typing it into the CLI.  The complete command must be followed by hitting `[RETURN]`

```bash
>debug 0
```

6. Perform a Wi-Fi scan to see the list of Access Points that are currently being detected by the board's Wi-Fi network controller.  Use  the `wifi` command's `scan` option by manually typing it into the CLI.  The complete command must be followed by hitting `[RETURN]`

```bash
>wifi -scan
```

7. Configure the SAM-IoT board's internal Wi-Fi settings with your wireless router’s SSID and password using the `wifi` command's `set` option by manually typing it into the CLI.  The complete command must be followed by hitting `[RETURN]` (there cannot be any spaces used in the SSID or password)

```bash
>wifi -set <NETWORK_SSID>,<PASSWORD>,<SECURITY_OPTION[1=Open|2=WPA|3=WEP]>
```

For example, if the SSID of the router is "MyWirelessRouter" and the WPA/WPA2 key is "MyRoutersPassword", the exact command to type into the CLI (followed by `[RETURN]`) would be

```bash
>wifi -set MyWirelessRouter,MyRoutersPassword,2
```

8. At the CLI prompt, type in the command `reset` and hit `[RETURN]` to restart the host application.  The Blue LED should eventually stay solidly ON to signify that the SAM-IoT board has successfully connected to the wireless router.

```bash
>reset
```

9. At this point, the board is connected to Wi-Fi, but has not yet established a connection with the cloud.  The `cloud` command can be used at any time to confirm the cloud connection status.  The complete command must be followed by hitting `[RETURN]`

```bash
>cloud -status
```

## Create an IoT Central Application for your Device

IoT Central allows you to create an application dashboard to monitor the telemetry and take appropriate actions based on customized rules.

1. Create a custom IoT Central application by starting with an existing [Microchip IoT Development Board Template](https://apps.azureiotcentral.com/build/new/19a49533-444f-488b-b183-82964f7e9272) (if there is a problem with loading the template, refer to the [Create an application](https://docs.microsoft.com/en-us/azure/iot-central/core/quick-deploy-iot-central) section to create your IoT Central application from scratch)

2. Review and select the settings for your IoT Central application (if needed, refer to [Create an application](https://docs.microsoft.com/en-us/azure/iot-central/core/quick-deploy-iot-central) for additional guidance on selecting the settings for your application). Click the `Create` button only after taking into consideration the following recommendations:

   - Choose a unique `Application name` which will result in a unique `URL` for accessing your application. Azure IoT Builder will populate a suggested unique `Application name` which can/should be leveraged, resulting in a unique `URL`.

     <img src=".//media/image80a.png" style="width:5.5.in;height:2.53506in"/>

   - If you select the **Free** plan, you can connect up to 5 devices.  However, the free trial period will expire after 7 days which means a [paid pricing plan](https://azure.microsoft.com/en-us/pricing/details/iot-central/) will need to be selected to continue using the application.  Of course, there is nothing to stop you from creating a new free trial application but the device will need to be configured for the app from scratch.  Since the **Standard** plans each allow 2 free devices with no time-restricted trial period, if you only plan on evaluating 1 or 2 devices for connecting to the IoT Central app, then it's best to choose the **Standard 2** plan to get the highest total allowable number of messages (30K per month).

     <img src=".//media/image80b.png" style="width:6.5.in;height:3.63506in" />

   - `Billing info` section: If there is an issue with selecting an existing subscription in the drop-down list (or no subscriptions appear in the list at all), click on the `Create subscription` link to create a new subscription to use for the creation of this application.

     <img src=".//media/image80c.png" style="width:6.5.in;height:2.53506in"/>

3. Create an X.509 enrollment group for your IoT Central application. If not already opened, launch your IoT Central application and navigate to `Administration` in the left pane and select `Device connection`

4. Select `+ Create enrollment group`, and create a new enrollment group using any name (Group type = `IoT devices`, attestation type = `Certificates (X.509)`).  Hit `Save` when finished

   <img src=".//media/image81.png" style="width:6.5.in;height:3.63506in" />

5. Now that the new enrollment group has been created, select `+ Manage Primary`.

   <img src=".//media/image82.png" style="width:5.5.in;height:2.53506in" />

6. Select the file/folder icon associated with the `Primary` field and upload the root certificate file `root-ca.crt` (located in the `ChainOfTrust` sub-folder that was created by the `SAM-IoT Provisioning Tools Package for Windows`).  The message "`(!) Needs verification`" should appear.  The `Subject` and `Thumbprint` fields will automatically populate themselves

   <img src=".//media/image75.png" style="width:5.5.in;height:3.13506in" />

7. Click `Generate verification code` (this code will be copied to the clipboard which will be needed in a future step)

   <img src=".//media/image76.png" style="width:6.5.in;height:2.03506in" />

8. Open a Git Bash window (Start menu &gt; type `Git Bash`)

   <img src=".//media/image15.png" style="width:3.21739in;height:0.94745in" />

9. Using the Git Bash command line, navigate to your certificates folder (the `ChainOfTrust` sub-folder which was generated by the [SAM-IoT Provisioning Tools Package for Windows](https://github.com/randywu763/sam-iot-provision))

   ```bash
   cd <path>\sam-iot-provision-main\SAM_IoT_Certs_Generator\ChainOfTrust
   ```

10. Execute the below command in the Git Bash window (copy and paste for best results)

    **Note**: Once you enter the below command, you will then be asked to enter information for various fields that will be incorporated into your certificate request. Enter the verification code (which was just generated previously) when prompted for the `Common Name`. It's recommended to just copy the `Verification code` to the clipboard and paste it when it's time to enter the `Common Name`.  For the rest of the fields, you can enter anything you want (or just hit `[RETURN]` to keep them blank which is fine for basic demonstration purposes).  If you accidentally hit `[RETURN]` when asked for the `Common Name`, you will need to run the command again...

    ```bash
    openssl req -new -key root-ca.key -out azure_root_ca_verification.csr
    ```

    <img src=".//media/image16.png" style="width:5.in;height:3.18982in" />

11. Generate the verification certificate by executing the following command exactly as shown (suggest copy and paste for best results)

    ```bash
    openssl x509 -req -in azure_root_ca_verification.csr -CA root-ca.crt -CAkey root-ca.key -CAcreateserial -out azure_signer_verification.cer -days 365 -sha256
    ```

12. Click `Verify` and select the `azure_signer_verification.cer` file to upload.  Confirm that the `Primary` certificate has been verified and that a `Thumbprint` has been generated for your certificate.  Click on `Close` to exit the current window, then click on `Save` at the top of the web application window.  The X.509 enrollment group has been successfully created and should be ready to go!

    <img src=".//media/image83.png" style="width:5.in;height:2.18982in" alt="A screenshot of a cell phone Description automatically generated" />

13. If not already active, launch a terminal emulator window and connect to the COM port corresponding to the SAM-IoT board at `9600` baud (**disable** local echo for the terminal settings for best results).  Hit `[RETURN]` to bring up the Command Line Interface prompt (which is simply the `>` character). Type `help` and then hit `[RETURN]` to get the list of available commands for the CLI.  The Command Line Interface allows you to send simple ASCII-string commands to set or get the user-configurable operating parameters of the application while it is running

    <img src=".//media/image44.png" style="width:5.in;height:3.18982in" alt="A screenshot of a cell phone Description automatically generated" />

14. Look up the `ID Scope` for the DPS created/used by your IoT Central application (navigate to your application's web page and using the left-hand navigation pane, select `Administration` > `Device connection`).  The `ID Scope` will be programmed/saved into the [ATECC608A](https://www.microchip.com/wwwproducts/en/atecc608a) secure element on the board in the next step

    <img src=".//media/image84.png" style="width:5.in;height:3.18982in" />

15. In the terminal emulator window, hit `[RETURN]` to bring up the Command Line Interface prompt (which is simply the `>` character>). At the CLI prompt, type in the `idscope <your_ID_scope>` command to set it (which gets saved in the [ATECC608A](https://www.microchip.com/wwwproducts/en/atecc608a) secure element on the board) and then hit `[RETURN]`.  The ID Scope can be read out from the board by issuing the `idscope` command without specifying any parameter on the command line

    <img src=".//media/image85.png" style="width:5.in;height:3.18982in" />

16. In the terminal emulator window, hit `[RETURN]` to bring up the CLI prompt. Type in the command `reset` and hit `[RETURN]`

17. Wait for the SAM-IoT board to connect to your IoT Central application’s DPS; the Blue and Green LEDs will be flashing and/or staying on at different times/rates (which could take up to a few minutes).  Eventually the Blue and Green LEDs should both remain constantly ON.

    NOTE: If the Red LED comes on, then something may have been incorrectly programmed (e.g. wrong firmware, ID scope was entered incorrectly, etc.)

18. At this point, the board should have established a valid cloud connection (this can be confirmed visually by the Green LED staying on constantly).  The `cloud` command can be used at any time to confirm the cloud connection status using the CLI.  The complete command must be followed by hitting `[RETURN]`

    ```bash
    >cloud -status
    ```

19. Go back to your web browser to access the Azure IoT Central application.  Use the left-hand side pane and select `Devices` > `All Devices`.  Confirm that your device is listed – the device name & ID is the Common Name of the device certificate (which should be `sn + {17-digit device ID}`)

    <img src=".//media/image86.png" style="width:5.in;height:1.38982in" alt="A screenshot of a cell phone Description automatically generated" />





## Validate connection

Whenever the SW1 button is pressed on the PIC-IoT board, the red LED will toggle whenever a data frame has been sent out on the SPI bus.  Whenever a valid data frame has been received and properly decoded by the SAM-IoT, its yellow LED will toggle.

NOTE: It is strongly recommended to issue the `telemetry 0,0` command on the CLI prior to sending data frames out to the DTI.  It is not a requirement, but issuing this command will ensure that the DTI receive buffer has been properly reset to receive a data frame

<img src=".//media/image131.png"/>





## Data Visualization using the IoT Central Application

Using the dashboard drop-down selection list, choose the `SAM-IoT WM v2 Properties & Telemetry Data Buckets` dashboard in the IoT Central Template Application to see the last known received values for each of the 4 telemetry string values.

Note: you may need to configure each of the property tiles in the premade IoT Central Template application to the device you just connected and validated. To do this, click the "Edit" button at the top of your dashboard, then click "Configure" (gear icon) on each desired property tile. Then, select your device group (in this case, `SAM-IoT WM v2`) and your device as listed before (which should be `sn + {17-digit device ID}`).

![image132](.//media/image132.png)



Here are a list of suggested properties to include in the dashboard (see screenshot below for visual):

| System Information                                           | App MCU Properties                                           | LED States                                                   |                              |
| ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ | ---------------------------- |
| ATWINC1510 Firmware Version<br />Debug Level<br/>Disable Telemetry<br/>IP Address<br/>Set Telemetry Interval | App MCU Property 1<br/>App MCU Property 2<br/>App MCU Property 3<br/>App MCU Property 4 | Blue LED state<br/>Green LED state<br/>Red LED state<br/>Yellow LED state |                              |
| **Telemetry Data Bucket #1**                                 | **Telemetry Data Bucket #2**                                 | **Telemetry Data Bucket #3**                                 | **Telemetry Data Bucket #4** |
| App MCU Telemetry 1 (String)                                 | App MCU Telemetry 2 (String)                                 | App MCU Telemetry 3 (String)                                 | App MCU Telemetry 4 (String) |

<img src=".//media/image126.png"/>



### Confirmation of Reception for Every Data Frame Sent by the Application Processor

Observe each of the raw data messages received by the IoT Central application by clicking on the device and then selecting the `Raw Data` tab.  Note that each message contains a time stamp that can be used to verify the exact time each individual message was received by the IoT Central application.

<img src=".//media/image127.png"/>

<img src=".//media/image128.png"/>

<img src=".//media/image129.png"/>
