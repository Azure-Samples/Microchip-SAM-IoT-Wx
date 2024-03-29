/**
 * \file
 *
 * \brief  Microchip Crypto Auth device object
 *
 * \copyright (c) 2015-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \page License
 * 
 * Subject to your compliance with these terms, you may use Microchip software
 * and any derivatives exclusively with Microchip products. It is your
 * responsibility to comply with third party license terms applicable to your
 * use of third party software (including open source software) that may
 * accompany Microchip software.
 * 
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
 * PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT,
 * SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE
 * OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF
 * MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE
 * FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL
 * LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED
 * THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR
 * THIS SOFTWARE.
 */

#ifndef ATCA_DEVICE_H
#define ATCA_DEVICE_H

#include "atca_command.h"
#include "atca_iface.h"
/** \defgroup device ATCADevice (atca_)
   @{ */

#ifdef __cplusplus
extern "C" {
#endif

/** \brief atca_device is the C object backing ATCADevice.  See the
 *         atca_device.h file for details on the ATCADevice methods.
 */
struct atca_device
{
    ATCACommand mCommands;   //!< Command set for a given CryptoAuth device
    ATCAIface   mIface;      //!< Physical interface
};

typedef struct atca_device* ATCADevice;

ATCA_STATUS initATCADevice(ATCAIfaceCfg* cfg, ATCADevice cadev);
ATCADevice  newATCADevice(ATCAIfaceCfg* cfg);
ATCA_STATUS releaseATCADevice(ATCADevice ca_dev);
void        deleteATCADevice(ATCADevice* ca_dev);

ATCACommand atGetCommands(ATCADevice dev);
ATCAIface   atGetIFace(ATCADevice dev);


#ifdef __cplusplus
}
#endif
/** @} */
#endif
