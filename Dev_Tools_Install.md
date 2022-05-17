# Development Tools Installation for SAM-IoT WG Development Board (Part No. EV75S95A)

Please install the following software in the exact order listed:

1. Install the following components that make up the Microchip `MPLAB X` tool chain for embedded code development on 32-bit MCU platforms

    - [MPLAB X IDE](https://www.microchip.com/mplab/mplab-x-ide)

    - [MPLAB XC32 Compiler](https://www.microchip.com/en-us/development-tools-tools-and-software/mplab-xc-compilers#tabs)

        NOTE: The last known XC32 compiler version which successfully builds this demonstration project was version `4.00`.  If the currently available download version is greater than this last known version and does not successfully build the provided demo project, download the last known working XC32 version from the [MPLAB Development Ecosystem Downloads Archive](https://www.microchip.com/en-us/tools-resources/archives/mplab-ecosystem)

    - [MPLAB Harmony Software Framework](https://microchipdeveloper.com/harmony3:mhc-overview)

2. Install [Git](https://git-scm.com) (a free and open source distributed version control system) by executing the following steps:

- Download/install the latest version of [Git for Windows, macOS, or Linux](https://git-scm.com/downloads)

- Verify working operation of the `Git Bash` prompt (e.g. for Windows: click `Start` > type `Git Bash`)

    <img src=".//media/image15.png"/>

3. Install the Microsoft [Azure Command Line Interface (CLI)](https://docs.microsoft.com/cli/azure/?view=azure-cli-latest). The Azure CLI is a set of commands used to create and manage Azure resources. The Azure CLI is available across Azure services and is designed to get you working quickly with Azure, with an emphasis on automation

4. Install the latest [Python](https://www.python.org) release. During the installation setup, be sure to enable/check the option for `Add Python <X.Y> to PATH`

5. Install and verify [pyazureutils](https://pypi.org/project/pyazureutils/) (a Microchip utility for interacting with Microsoft Azure web services via the Azure CLI) by executing the following steps:

   - Launch a `Command Prompt` or `PowerShell` window (e.g. for Windows: click on `Start` > type `PowerShell` in the Search field > `Open`)
   - Execute the following command line to install the utility program

        ```shell
        pip install --upgrade pyazureutils
        ```
   - Bring up the IoT Central's specific help menu to verify that the `pyazureutils.exe` program can be found
 
        ```shell
        pyazureutils iotcentral --help
        ```

        NOTE: If the `pyazureutils` command cannot be found nor executed, search for the location of the `pyazureutils.exe` program and add the absolute path of its location to the Windows `PATH` environment variable. Launch a new command line window and try bringing up the help menu again

