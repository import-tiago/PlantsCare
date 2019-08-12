/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//  MSP430FR413x Demo - Configure MCLK for 16MHz operation, and XT1 sourcing
//                                     FLLREF and ACLK.
//
//  Description: Configure MCLK for 16MHz. FLL reference clock is XT1. At this
//                    speed, the FRAM requires wait states.
//                    ACLK = XT1 ~32768Hz, SMCLK = MCLK = 16MHz.
//                    Toggle LED to indicate that the program is running.
//
//           MSP430FR4133
//         ---------------
//        |               |
//        |            XIN|--
//     /|\|               |  ~32768Hz
//      | |           XOUT|--
//      --|RST            |
//        |               |
//        |          P1.0 |--> LED
//        |          P1.4 |---> MCLK = 1 MHz
//        |               |
//        |               |
//
//
//   Kathryn Adamsky
//   Texas Instruments Inc.
//   June 2016
//   Built with IAR Embedded Workbench v6.10.2 & Code Composer Studio v6.1.3
//******************************************************************************
#include <msp430.h>

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                    // Stop watchdog timer

    P4SEL0 |= BIT1 | BIT2;                  // set XT1 pin as second function

    do
    {
        CSCTL7 &= ~(XT1OFFG | DCOFFG);           // Clear XT1 and DCO fault flag
        SFRIFG1 &= ~OFIFG;
    }while (SFRIFG1 & OFIFG);                    // Test oscillator fault flag

    __bis_SR_register(SCG0);                     // disable FLL

    CSCTL3 |= SELREF__XT1CLK;                    // Set XT1CLK as FLL reference source
    CSCTL0 = 0;                                  // clear DCO and MOD registers

    __bic_SR_register(SCG0);                     // enable FLL

    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));   // Poll until FLL is locked

    CSCTL7 &= ~DCOFFG;                           // Clear DCO fault flag
    CSCTL4 = SELMS__DCOCLKDIV | SELA__XT1CLK;    // set ACLK = XT1CLK = 32768Hz and DCOCLK = MCLK and SMCLK source
    CSCTL4 = SELMS__DCOCLKDIV | SELA__XT1CLK;    // set XT1 (~32768Hz) as ACLK source, ACLK = 32768Hz
                                                 // default DCOCLKDIV as MCLK and SMCLK source

    P1DIR |= BIT5 + BIT4;                        // set MCLK and LED pin as output
    P1SEL0 |= BIT4;                              // set MCLK pin as second function

    PM5CTL0 &= ~LOCKLPM5;                        // Disable the GPIO power-on default high-impedance mode
                                                 // to activate previously configured port settings

    while(1)
    {
        P1OUT ^= BIT5;                           // Toggle P1.0 using exclusive-OR
        __delay_cycles(8000000);
    }
}
