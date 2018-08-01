#!/usr/bin/env python
#######################  PEXPECT  -  S k r i p t  ##########################
# 
#       Project: A404 FPGA Design Test
#
#        Author: dieter.pfeuffer@men.de
#         $Date: $
#     $Revision: $
#
#    Description: Automatic test for A404 FPGA design verification
#
#                 Required HW configuration
#                 -------------------------
#                 - A15 CPU running VxWorks
#                 - old CME board or A404
#                 - loop-back cable: TX1->RX2, TX2->RX1
#                 
#-------------------------------[ History ]---------------------------------
#
# $Log: test_fpga.py,v $
############################################################################

import sys
import os
import unittest
import re
import time
import string

from datetime import date

import pexpect

import log4py
from log4py import Logger

import common
from common import *

import a404
from a404 import *

import testparams
from testparams import *

import log4pyConf
from log4pyConf import log        
            
#-----------------------------------
# CLASS DEFINITION
#-----------------------------------
class a404Test(unittest.TestCase):
   
  ##### member function #####
  def setUp(self):
    "executed before each test_xxx() routine"
    # create CPU instance
    self.cpu1 = CPU("A15#1",cpu_1_ip,self)
    
    # open console to CPU
    self.cpu1.open()

    print ""
    
    # create A404 instance and detect the board
    self.brd1 = A404("a404#1",a404_1_scarte,a404_1_a32off,self.cpu1)
 
  ##### member function #####
  def tearDown(self):
    "executed after each test_xxx() routine"
    time.sleep(1)

    # close
    self.cpu1.close()
        
    # seems to be required after close on cygwin...
    time.sleep(1)

  ##### member function #####
  def trxData1_2(self,txCh,rxCh,t):
    "write data to TX1/2 space, receive and check RX1/2"
    brd = self.brd1

    # not A404_ENH_MODE
    if brd.skipIfBrd( 'A404_ENH_MODE','TX1/TX2 write' ):
      return

    if rxCh == 'rx1' :
      rxstat = 1
    else :
      rxstat = 2
         
    brd.clrRx( rxCh )    
    brd.tx( txCh, t )    
    brd.rx( rxCh, t )
    brd.checkClrRxstat( rxstat, RXSTAT_CONNECT | RXSTAT_FFFULL_NO )      
      
  ##### member function #####
  def trxData12(self,t):
    "write data to TX12 space, receive and check RX1/RX2"
    brd = self.brd1

    brd.clrRx( 'rx1' )    
    brd.clrRx( 'rx2' )    
    brd.tx( 'tx12', t )    
    brd.rx( 'rx1', t )    
    brd.rx( 'rx2', t )

  ##### member function #####
  def trxBlkData12(self,mod):
    "blk write data to TX12 space, blk receive and check RX1/RX2"
    brd = self.brd1
        
    brd.clrRx( 'rx1' )    
    brd.clrRx( 'rx2' )    
    brd.txBlk( mod )
    brd.rxBlk( 'rx1',mod )
    brd.rxBlk( 'rx2',mod )
      
  ##### member function #####
  def irqBasicTest(self,txCh,rxCh,irqLev,irqVec):
    "IRQ basic test"
    brd = self.brd1
    cpu = brd.cpu

    cmmd = "a404RxSetIrqVector( 0x%x, %d, %d, %d )" % \
      (brd.base, brd.enh, rxCh, irqVec )
    log.info("RX%d: set vector=%d" % (rxCh, irqVec))
    execCmdSucc(cpu.cons, cmmd, 0)

    cmmd = "a404RxIrqEnable( 0x%x, %d, %d, %d, 1 )" % \
      (brd.base, brd.enh, rxCh, irqLev )
    log.info("RX%d: enable level=%d" % (rxCh, irqLev))
    execCmdSucc(cpu.cons, cmmd, 0)
    
    cmmd = "a404InstallIsr( %d, %d )" % \
      (irqVec, irqLev)
    log.info("CPU: install ISR for vector=%d, enable level=%d" %
      (irqVec,irqLev))
    execCmdSucc(cpu.cons, cmmd, 0)

    cmmd = "a404GetClrIsrCount( %d )" % irqVec
    log.info("CPU: get/clr ISR count for vector=%d" % irqVec)
    execCmdSucc(cpu.cons, cmmd, 0)

    cmmd = "a404TxIrqSend( 0x%x, %d, %d, %d )" % \
      (brd.base, brd.enh, txCh, irqLev )
    log.info("TX%d: send IRQ with level=%d" % (txCh, irqLev))
    execCmdSucc(cpu.cons, cmmd, 0)

    cmmd = "a404GetClrIsrCount( %d )" % irqVec
    log.info("CPU: get/clr ISR count for vector=%d" % irqVec)
    execCmdSucc(cpu.cons, cmmd, 1)

    cmmd = "sysIntDisable( %d )" % irqLev
    log.info("CPU: disable level=%d" % irqLev)
    execCmdSucc(cpu.cons, cmmd, 0)

    cmmd = "a404RxGetIrqPending( 0x%x, %d, %d, %d )" % \
      (brd.base, brd.enh, rxCh, irqLev )
    log.info("RX%d: check for pending level=%d" % (rxCh, irqLev))
    execCmdSucc(cpu.cons, cmmd, 0)

    cmmd = "a404TxIrqSend( 0x%x, %d, %d, %d )" % \
      (brd.base, brd.enh, txCh, irqLev )
    log.info("TX%d: send IRQ with level=%d" % (txCh, irqLev))
    execCmdSucc(cpu.cons, cmmd, 0)

    cmmd = "a404RxGetIrqPending( 0x%x, %d, %d, %d )" % \
      (brd.base, brd.enh, rxCh, irqLev )
    log.info("RX%d: check for pending level=%d" % (rxCh, irqLev))
    execCmdSucc(cpu.cons, cmmd, 1)

    cmmd = "a404ByteRegWrite( 0x%x, %d, %d, 0x%x, 0x%x )" % \
      (brd.base, brd.enh, rxCh, RXIRQ, 0xff)  
    log.info("RX%d: clear all pending IRQs" % rxCh)
    execCmdSucc(cpu.cons, cmmd, 0)

    cmmd = "a404RxGetIrqPending( 0x%x, %d, %d, %d )" % \
      (brd.base, brd.enh, rxCh, irqLev )
    log.info("RX%d: check for pending level=%d" % (rxCh, irqLev))
    execCmdSucc(cpu.cons, cmmd, 0)

    cmmd = "sysIntEnable( %d )" % irqLev
    log.info("CPU: enable level=%d" % irqLev)
    execCmdSucc(cpu.cons, cmmd, 0)

    cmmd = "a404GetClrIsrCount( %d )" % irqVec
    log.info("CPU: get/clr ISR count for vector=%d" % irqVec)
    execCmdSucc(cpu.cons, cmmd, 0)
    
    cmmd = "a404RxIrqEnable( 0x%x, %d, %d, %d, 0 )" % \
      (brd.base, brd.enh, rxCh, irqLev )
    log.info("RX%d: disable level=%d" % (rxCh, irqLev))
    execCmdSucc(cpu.cons, cmmd, 0)    

    cmmd = "a404TxIrqSend( 0x%x, %d, %d, %d )" % \
      (brd.base, brd.enh, txCh, irqLev )
    log.info("TX%d: send IRQ with level=%d" % (txCh, irqLev))
    execCmdSucc(cpu.cons, cmmd, 0)
    
    cmmd = "a404GetClrIsrCount( %d )" % irqVec
    log.info("CPU: get/clr ISR count for vector=%d" % irqVec)
    execCmdSucc(cpu.cons, cmmd, 0)  

    cmmd = "a404RxGetIrqPending( 0x%x, %d, %d, %d )" % \
      (brd.base, brd.enh, rxCh, irqLev )
    log.info("RX%d: check for pending level=%d" % (rxCh, irqLev))
    execCmdSucc(cpu.cons, cmmd, 0)

    cmmd = "a404ByteRegWrite( 0x%x, %d, %d, 0x%x, 0x%x )" % \
      (brd.base, brd.enh, rxCh, RXIRQ, 0xff)  
    log.info("RX%d: clear all pending IRQs" % rxCh)
    execCmdSucc(cpu.cons, cmmd, 0)

    cmmd = "a404UnInstallIsr( %d )" % \
      (irqVec)
    log.info("CPU: uninstall ISR for vector=%d" %
      (irqVec))
    execCmdSucc(cpu.cons, cmmd, 0)

  ##### member function #####
  def membench(self,space,txoff):
    "membench to TX12 space"
    brd = self.brd1
    c = brd.cpu.cons

    if txoff :
      onOff = 'off'
    else :
      onOff = 'on'

	# A24 space
    if space == 'a24' :
      sglMaxblkTbl = [0x00400, 0x01000, 0x04000, 0x10000, 0x20000]
      blkModTbl = ['a24d16','a24d32']

      log.info("a24d16 single access to TX12 space (tx1/tx2 %s)" % onOff)
      brd.setA24Base( 'A24D16' )
      for blkSize in sglMaxblkTbl:
        c.sendline( "membench2( 0x%x, 0x%x, 0x%x, 0x%x )" \
          % (brd.base + brd.TX12, blkSize, blkSize, brd.TRX_SIZE) )
        c.expect( 'value = 0 = 0x0')
        c.expect( shPrompt )  
      brd.setA24Base( 'A24D32' )

    # A32 space
    else :
      sglMaxblkTbl = [0x00400, 0x01000, 0x04000, 0x10000, 0x40000, 0x100000]
      # a32d64 currently unsupported blkModTbl = ['a32d32','a32d64']
      blkModTbl = ['a32d32']
 
    log.info("%sd32 single access to TX12 space (tx1/tx2 %s)" % (space, onOff))
    for blkSize in sglMaxblkTbl:
      c.sendline( "membench2( 0x%x, 0x%x, 0x%x, 0x%x )" \
        % (brd.base + brd.TX12, blkSize, blkSize, brd.TRX_SIZE) )
      c.expect( 'value = 0 = 0x0')
      c.expect( shPrompt )  
   	  
    for mod in blkModTbl:
      log.info("%s burst access to TX12 space (tx1/tx2 %s)" % (mod, onOff))
      c.sendline( "vmeblkbench( 0x%x, \"%s\", 0x%x, 0x%x, 0x%x )" \
        % (brd.vmeAddr + brd.TX12, mod, 0x400, brd.TRX_SIZE, brd.TRX_SIZE) )
      c.expect( 'value = 0 = 0x0')
      c.expect( shPrompt )  

  #-----------------------------------
  # TEST ROUTINES
  #-----------------------------------
 
  #---------- VME-Register access ----------
  vmeRegTbl = {'RXMASQ1': 0x20009, 
               'RXMASQ2': 0x20019,
               'RXVECT1': 0x2000B,
               'RXVECT2': 0x2001B}
                  
  def test_200_VMEREGS_A24(self):
    log.warn("=== A24 access to VME-Registers ===")
    brd = self.brd1
    cpu = brd.cpu

    for regname, offs in self.vmeRegTbl.items():
      startaddr = brd.base + brd.RXC + offs
      cmmd = "mtest \"0x%x 0x%x -n=3 -o=2 -q=%d -t=%s\"" % \
        (startaddr, startaddr+1, mtestMaxErr, brd.accOpt)
      log.info("access %s" % regname)
      execCmdSucc(cpu.cons, cmmd, 0)
      
  def test_210_VMEREGS_A32(self):       
    log.warn("=== A32 access to VME-Registers ===")
    brd = self.brd1
    cpu = brd.cpu
    
    # A404_ENH_MODE only
    if brd.skipIfNotBrd( 'A404_ENH_MODE','test' ) :
      return
    
    brd.enableA32()           

    for regname, offs in self.vmeRegTbl.items():
      startaddr = brd.base + brd.RXC + offs
      cmmd = "mtest \"0x%x 0x%x -n=3 -o=2 -q=%d -t=%s\"" % \
        (startaddr, startaddr+1, mtestMaxErr, brd.accOpt)
      log.info("access %s" % regname)
      execCmdSucc(cpu.cons, cmmd, 0)
      
    brd.disableA32()          

  #---------- SDRAM access ----------
  def sdramRxTbl(self,brd):
    return {'RX#1 space': brd.RX1, 
            'RX#2 space': brd.RX2}
  
  def test_300_SDRAM_A24(self):
    log.warn("=== A24 access to SDRAM ===")
    brd = self.brd1
    cpu = brd.cpu
                    
    for spacename, offs in self.sdramRxTbl(brd).items():
      startaddr = brd.base + offs
      endaddr = startaddr + brd.TRX_SIZE - 1
      cmmd = "mtest \"0x%x 0x%x -n=2 -o=2 -q=%d -t=%s\"" % \
        (startaddr, endaddr, mtestMaxErr, brd.accOpt)
      log.info("access %s" % spacename)
      execCmdSucc(cpu.cons, cmmd, 0)      
      
  def test_305_SDRAM_A24BLK(self):
    log.warn("=== A24BLK access to SDRAM ===")
    brd = self.brd1
    cpu = brd.cpu

    # A404 only
    if brd.skipIfBrd( 'OLD_CME','test' ) :
      return
                    
    for spacename, offs in self.sdramRxTbl(brd).items():
      startaddr = brd.vmeAddr + offs
      endaddr = startaddr + brd.TRX_SIZE - 1
      log.info("access %s" % spacename)
      # D16
      cmmd = "mtest \"0x%x 0x%x -n=2 -o=2 -q=%d -t=v -m=a24d16,10\"" % \
        (startaddr, endaddr, mtestMaxErr)
      execCmdSucc(cpu.cons, cmmd, 0)      
      # D32
      cmmd = "mtest \"0x%x 0x%x -n=2 -o=2 -q=%d -t=v -m=a24d32,10\"" % \
        (startaddr, endaddr, mtestMaxErr)
      execCmdSucc(cpu.cons, cmmd, 0)      
      
  def test_310_SDRAM_A32(self):
    log.warn("=== A32 access to SDRAM (A404 enhanced mode) ===")
    brd = self.brd1
    cpu = brd.cpu
    
    # A404_ENH_MODE only
    if brd.skipIfNotBrd( 'A404_ENH_MODE','test' ) :
      return
      
    brd.enableA32()           
                    
    for spacename, offs in self.sdramRxTbl(brd).items():
      startaddr = brd.base + offs
      endaddr = startaddr + brd.TRX_SIZE - 1
      cmmd = "mtest \"0x%x 0x%x -n=3 -o=2 -q=%d -t=%s\"" % \
        (startaddr, endaddr, mtestMaxErr, brd.accOpt)
      log.info("access %s" % spacename)
      execCmdSucc(cpu.cons, cmmd, 0)
      
    brd.disableA32()              

  def test_315_SDRAM_A32BLK(self):
    log.warn("=== A32BLK access to SDRAM (A404 enhanced mode) ===")
    brd = self.brd1
    cpu = brd.cpu

    # A404_ENH_MODE only
    if brd.skipIfNotBrd( 'A404_ENH_MODE','test' ) :
      return
      
    brd.enableA32()           
                                        
    for spacename, offs in self.sdramRxTbl(brd).items():
      startaddr = brd.vmeAddr + offs
      endaddr = startaddr + brd.TRX_SIZE - 1
      log.info("access %s" % spacename)
      # D32
      cmmd = "mtest \"0x%x 0x%x -n=2 -o=2 -q=%d -t=v -m=a32d32,10\"" % \
        (startaddr, endaddr, mtestMaxErr)
      execCmdSucc(cpu.cons, cmmd, 0)
      # D64
      cmmd = "mtest \"0x%x 0x%x -n=2 -o=2 -q=%d -t=v -m=a32d64,10\"" % \
        (startaddr, endaddr, mtestMaxErr)
      execCmdSucc(cpu.cons, cmmd, 0)

    brd.disableA32()              

  def test_320_SDRAM_TX12_DUPLICATION(self):
    log.warn("=== TX12 duplication to TX1/TX2 ===")
    brd = self.brd1
    cpu = brd.cpu

    # not A404_ENH_MODE
    if brd.skipIfBrd( 'A404_ENH_MODE','test' ):
      return

    brd.clrRx( 'tx1' )
    brd.clrRx( 'tx2' )
    
    brd.tx( 'tx12', 'w' )    

    brd.rx( 'tx1', 'w' )    
    brd.rx( 'tx2', 'w' )

  #---------- LEDS ----------
  def test_400_LEDS(self):
    log.warn("=== toggle LEDs ===")
    brd = self.brd1
    cpu = brd.cpu

    # A404 only
    if brd.skipIfBrd( 'OLD_CME','test' ) :
      return

    brd.disableTX1TX2()
    brd.getClrRxstat(1)
    brd.getClrRxstat(2)   

    cmmd = "a404ToggleLeds(0x%x, %d)" % (brd.base, brd.enh)
    userReady("Please verify the coming LED states:\n" \
    " 1. ch#1: LEDs goes/remain ON in the order: rx-red, rx-yellow, rx-green, tx-yellow, tx-green\n" \
    " 2. ch#1: all LEDs restored to origin states\n" \
    " 3. step 1..2 repeated for ch#2")
    execCmdSucc(cpu.cons, cmmd, 0) 
    brd.enableTX1TX2()
    userVerified(self)
    
  #---------- Data Transmission ----------
  def _TRXD_BASIC(self,loop):
    brd = self.brd1
    cpu = brd.cpu

    brd.initBoard(loop)        
    if loop :
      ret = 1
    else :
      ret = 2	  

    if brd.enh == 1 :
      ret = 0
      
    log.info("check basic data transfer")
    cmmd = "a404CheckLanes( 0x%x, %d )" % \
      (brd.base, brd.enh )
    execCmdSucc(cpu.cons, cmmd, ret)
    brd.checkClrRxstat12( RXSTAT_CONNECT | RXSTAT_FFFULL_NO )

    if loop :
      brd.disableHwLoop()

  def test_500_TRXD_BASIC(self):
    log.warn("=== A24 TRX data: basic transfer test ===")     
    self._TRXD_BASIC(0)

  def test_510_TRXD_BASIC_LOOP(self):
    log.warn("=== HW-LOOP - A24 TRX data: basic transfer test ===")     
    brd = self.brd1

    # A404 only
    if brd.skipIfBrd( 'OLD_CME','test' ) :
      return
      
    self._TRXD_BASIC(1)

  def _TRXD_A24(self,loop):
    brd = self.brd1
    cpu = brd.cpu  

    brd.initBoard(loop)        

    for t in brd.accTbl :
      log.warn("---------- %s ----------" % tInfo(t))     
      if loop :
        self.trxData1_2('tx1','rx1',t)
        self.trxData1_2('tx2','rx2',t)
      else :
        self.trxData1_2('tx1','rx2',t)
        self.trxData1_2('tx2','rx1',t)      	

      self.trxData12(t)
      brd.checkClrRxstat12( RXSTAT_CONNECT | RXSTAT_FFFULL_NO )

    # A404 only
    if 0 == brd.skipIfBrd( 'OLD_CME','blk test' ) :    
      for mod in ['a24d16','a24d32'] :  
        log.warn("---------- blk %s ----------" % mod)     
        self.trxBlkData12(mod)
        brd.checkClrRxstat12( RXSTAT_CONNECT | RXSTAT_FFFULL_NO )

    if loop :
      brd.disableHwLoop() 
  
  def test_520_TRXD_A24(self):
    log.warn("=== A24 TRX data: TX1/TX2/TX12 transfer test ===")     
    self._TRXD_A24(0)

  def test_525_TRXD_A24_LOOP(self):
    log.warn("=== HW-LOOP - A24 TRX data: TX1/TX2/TX12 transfer test ===")     
    brd = self.brd1
    
    # A404 only
    if brd.skipIfBrd( 'OLD_CME','test' ) :
      return

    self._TRXD_A24(1)

  def _TRXD_A32(self,loop):
    brd = self.brd1
    cpu = brd.cpu
    
    # A404_ENH_MODE only
    if brd.skipIfNotBrd( 'A404_ENH_MODE','test' ) :
      return
      
    brd.enableA32()           
    brd.initBoard(loop)        
    
    for t in brd.accTbl :
      log.warn("---------- %s ----------" % tInfo(t))     
      self.trxData12(t)
      brd.checkClrRxstat12( RXSTAT_CONNECT | RXSTAT_FFFULL_NO )
    
    for mod in ['a32d32','a32d64'] :  
      log.warn("---------- blk %s ----------" % mod)     
      self.trxBlkData12(mod)
      brd.checkClrRxstat12( RXSTAT_CONNECT | RXSTAT_FFFULL_NO )
    
    if loop :
      brd.disableHwLoop() 
    brd.disableA32()                
    
  def test_530_TRXD_A32(self):
    log.warn("=== A32 TRX data: TX12 transfer test ===")     
    self._TRXD_A32(0)
    
  def test_535_TRXD_A32_LOOP(self):
    log.warn("=== HW-LOOP - A32 TRX data: TX12 transfer test ===")     
    self._TRXD_A32(1)                           
    
  #---------- Command Transmission ----------
  cmmdTbl = [ 0x70, 0x52, 0x34, 0x16, 0x00 ]  

  def _TRXC_A24(self,loop):
    brd = self.brd1
    cpu = brd.cpu
     
    brd.initBoard(loop)        
    if loop :
      srcTbl  = [1,2]
      sinkTbl = [1,2]
    else :
      srcTbl  = [1,2]
      sinkTbl = [2,1]
    
    for src, sink in zip(srcTbl, sinkTbl):    
      log.info("transfer commands (TXCOM%d-->RXCOM%d)" % (src,sink) )
      for cmmd in self.cmmdTbl:  
        brd.sendCmd( src, cmmd )  
        brd.recvVeriCmd( sink, cmmd )

    brd.checkClrRxstat12( RXSTAT_CONNECT | RXSTAT_FFFULL_NO )

    if loop :
      brd.disableHwLoop()
     
  def test_600_TRXC_A24(self):
    log.warn("=== A24 TRX command: TX1/TX2 transfer test ===")     
    self._TRXC_A24(0)

  def test_605_TRXC_A24_LOOP(self):
    log.warn("=== HW-LOOP - A24 TRX command: TX1/TX2 transfer test ===")     
    brd = self.brd1

    # A404 only
    if brd.skipIfBrd( 'OLD_CME','test' ) :
      return
     
    self._TRXC_A24(1)
                    
  def test_610_TRXC_A32(self):
    log.warn("=== A32 TRX command: TX1/TX2 transfer test ===")     
    brd = self.brd1
    cpu = brd.cpu
        
    # A404_ENH_MODE only
    if brd.skipIfNotBrd( 'A404_ENH_MODE','test' ) :
      return
      
    brd.enableA32()           
    brd.initBoard(0)
        
    log.info("transfer commands (TXCOM1-->RXCOM2)" )
    for cmmd in self.cmmdTbl:  
      brd.sendCmd( 1, cmmd )  
      brd.recvVeriCmd( 2, cmmd )

    log.info("transfer commands (TXCOM2-->RXCOM1)" )
    for cmmd in self.cmmdTbl:  
      brd.sendCmd( 2, cmmd )  
      brd.recvVeriCmd( 1, cmmd )                    

    brd.checkClrRxstat12( RXSTAT_CONNECT | RXSTAT_FFFULL_NO )
    
    brd.disableA32() 
   
  #---------- IRQ Test ----------
  def test_700_IRQ_BASIC(self):
    log.warn("=== BASIC IRQ test ===")     
    brd = self.brd1
    
    brd.initBoard(0)

    self.irqBasicTest( 1, 2, 5, 102 )
    brd.checkClrRxstat12( RXSTAT_CONNECT | RXSTAT_FFFULL_NO )

  #---------- IRQ Test ----------
  def test_705_IRQ_BASIC_LOOP(self):
    log.warn("=== HW-LOOP - BASIC IRQ test ===")     
    brd = self.brd1

    # A404 only
    if brd.skipIfBrd( 'OLD_CME','test' ) :
      return
     
    brd.initBoard(1)         
    self.irqBasicTest( 1, 1, 5, 102 )
    brd.checkClrRxstat12( RXSTAT_CONNECT | RXSTAT_FFFULL_NO )
    brd.disableHwLoop()

  def test_710_IRQ_A24(self):
    log.warn("=== A24 IRQ stress test ===")     
    brd = self.brd1
    cpu = brd.cpu   
    
    brd.initBoard(0)
    
    log.info("IRQ test:")
    cmmd = "a404IrqTest( 0x%x, %d )" % (brd.base, brd.enh)
    execCmdSucc(cpu.cons, cmmd, 0)
    brd.checkClrRxstat12( RXSTAT_CONNECT | RXSTAT_FFFULL_NO )
      
  def test_720_IRQ_A32(self):
    log.warn("=== A32 IRQ stress test ===")     
    brd = self.brd1
    cpu = brd.cpu    

    # A404_ENH_MODE only
    if brd.skipIfNotBrd( 'A404_ENH_MODE','test' ) :
      return
      
    brd.enableA32()           
    brd.initBoard(0)
    
    log.info("IRQ test:")
    cmmd = "a404IrqTest( 0x%x, %d )" % (brd.base, brd.enh)
    execCmdSucc(cpu.cons, cmmd, 0)
    brd.checkClrRxstat12( RXSTAT_CONNECT | RXSTAT_FFFULL_NO )

    brd.disableA32() 
             
  #---------- RX/TX Status Test ----------
  def test_800_DISCON(self):
    log.warn("=== Disconnection test: get TX/RX status ===")     
    brd = self.brd1
    cpu = brd.cpu
    
    brd.initBoard(0)
        
    brd.checkClrRxstat12( RXSTAT_CONNECT | RXSTAT_FFFULL_NO )
        
    userReady("Please remove the TX1->RX2, TX2->RX1 wires")
                    
    for ch in [1,2]:
      ret = brd.getClrRxstat(ch)
      log.info("check RXSTAT%d for !CONNECTED" % ch )
      if ret & RXSTAT_CONNECT == RXSTAT_CONNECT:
        log.error(cmmd + ' : failed')
        self.fail("RXSTAT::CONNECTED bit set")

    # not OLD_CME
    if brd.skipIfBrd( 'OLD_CME','TX status check' ) == 0 :
    	
      brd.clrTxstat( 1 )
      brd.clrTxstat( 2 )      

      # write words to tx1/2
      brd.tx( 'tx12', 'w' )    

      brd.checkClrTxstat( 1, 0x01 )
      brd.checkClrTxstat( 2, 0x01 )

      brd.checkClrTxstat( 1, 0x00 )
      brd.checkClrTxstat( 2, 0x00 )

      for loop in [0,1]:    
        for n in range(0,5):
          brd.initBoard(loop)
          for ch in [1,2]:    
            ret = brd.getClrRxstat(ch)
            if ret & RXSTAT_CRCERR == RXSTAT_CRCERR:
              self.fail("RXSTAT::RCV_CRC_ERR bit set")
            if ret & RXSTAT_DATALOST == RXSTAT_DATALOST:
              self.fail("RXSTAT::RXSTAT_DATALOST bit set")

    userReady("Please re-plug the TX1->RX2, TX2->RX1 wires")

    for ch in [1,2]:
      ret = brd.getClrRxstat(ch)	
      log.info("check RXSTAT%d for CONNECTED" % ch )
      if ret & RXSTAT_CONNECT != RXSTAT_CONNECT:
        log.error(cmmd + ' : failed')
        self.fail("RXSTAT::CONNECTED bit not set")
  
  def test_810_CRCERR(self):
    log.warn("=== CRC error detection test: get RX status ===")     
    brd = self.brd1
    cpu = brd.cpu
    
    brd.initBoard(0)      

    log.info("TX1/TX2: enable CRC error insertion")
    for ch in [1,2]:
      cmmd = "a404ByteRegSetmask( 0x%x, %d, %d, 0x%x, 0x%x )" % \
        (brd.base, brd.enh, ch, TXCTRL, TXCTRL_CRCERR)  
      execCmdSucc(cpu.cons, cmmd, 0)
    
    # write 2 words to tx1/2
    brd.txn( 'tx12', 2, 'w' )    

    for ch in [1,2]:
      log.info("check RXSTAT%d for RCV_CRC_ERR (and clear)" % ch )
      
      # OLD_CME
      if brd.type == 'OLD_CME' :    
        log.info("OLD_CME: FIFO will be emptied by repeating the ""clear""")
              
        ret = brd.getClrRxstat(ch)
        if ret & RXSTAT_CRCERR != RXSTAT_CRCERR:
          self.fail("RXSTAT::RCV_CRC_ERR bit not set")

        ret = brd.getClrRxstat(ch)
        if ret & RXSTAT_CRCERR == RXSTAT_CRCERR:
          self.fail("RXSTAT::RCV_CRC_ERR bit set")
        if ret & RXSTAT_DATALOST != RXSTAT_DATALOST:
          self.fail("RXSTAT::RXSTAT_DATALOST bit not set")

        ret = brd.getClrRxstat(ch)

        ret = brd.getClrRxstat(ch)
        if ret & RXSTAT_CRCERR == RXSTAT_CRCERR:
          self.fail("RXSTAT::RCV_CRC_ERR bit set")
        if ret & RXSTAT_DATALOST == RXSTAT_DATALOST:
          self.fail("RXSTAT::RXSTAT_DATALOST bit set")

      # A404
      else:
        log.info("A404: Different behaviour as OLD_CME")
        
        ret = brd.getClrRxstat(ch)
        if ret & RXSTAT_CRCERR != RXSTAT_CRCERR:
          self.fail("RXSTAT::RCV_CRC_ERR bit not set")
        if ret & RXSTAT_DATALOST != RXSTAT_DATALOST:
          self.fail("RXSTAT::RXSTAT_DATALOST bit not set")
        
        ret = brd.getClrRxstat(ch)
        if ret & RXSTAT_CRCERR == RXSTAT_CRCERR:
          self.fail("RXSTAT::RCV_CRC_ERR bit set")
        if ret & RXSTAT_DATALOST == RXSTAT_DATALOST:
          self.fail("RXSTAT::RXSTAT_DATALOST bit set")      

    log.info("TX1/TX2: disable CRC error insertion")
    for ch in [1,2]:
      cmmd = "a404ByteRegClrmask( 0x%x, %d, %d, 0x%x, 0x%x )" % \
        (brd.base, brd.enh, ch, TXCTRL, TXCTRL_CRCERR)  
      execCmdSucc(cpu.cons, cmmd, 0)

    # write 2 words to tx1/2
    brd.txn( 'tx12', 2, 'w' )    

    brd.checkClrRxstat12( RXSTAT_CONNECT | RXSTAT_FFFULL_NO )

  def test_820_RXFIFO(self):
    log.warn("=== Receiver Fifo test: get RX status ===")     
    brd = self.brd1
    cpu = brd.cpu    

    # A404 only
    if brd.skipIfBrd( 'OLD_CME','test' ) :
      return

    brd.initBoard(0)

    for rxCh in ( 1, 2 ):
      log.info("RX%d: Receiver Fifo test" % rxCh)
      cmmd = "a404RxFifoTest( 0x%x, %d, %d )" % \
        (brd.base, brd.enh, rxCh)  
      execCmdSucc(cpu.cons, cmmd, 0)

  #---------- Simple Benchmark Test ---------- 
  def _BENCH(self,loop,txoff):
    brd = self.brd1
    cpu = brd.cpu

    # A404 only
    if brd.skipIfBrd( 'OLD_CME','test' ) :
      return

    brd.initBoard(loop)        
    if txoff :
      brd.disableTX1TX2()
    oldTimeout = cpu.cons.timeout
    cpu.cons.timeout = 60

    # A24
    self.membench('a24',txoff)  
    brd.checkClrRxstat12( RXSTAT_CONNECT | RXSTAT_FFFULL_NO )   

    # A404_ENH_MODE only
    if 0 == brd.skipIfNotBrd( 'A404_ENH_MODE','test' ) :  
      brd.enableA32()
      # A32  
      self.membench('a32',txoff)  
      brd.checkClrRxstat12( RXSTAT_CONNECT | RXSTAT_FFFULL_NO )
      brd.disableA32() 

    if loop :
      brd.disableHwLoop()   
    if txoff :
      brd.enableTX1TX2()
    cpu.cons.timeout = oldTimeout

  def test_900_BENCH(self):
    log.warn("=== simple benchmark test ===")
    self._BENCH(0,0)
    self._BENCH(0,1)

  def test_910_BENCH_LOOP(self):
    log.warn("=== HW-LOOP - simple benchmark test ===")
    self._BENCH(1,0)
    self._BENCH(1,1)
                           
#----------------------------------------------------------------------
if __name__ == '__main__':

    def baseSuite():
      "basic suite: tests without TX/RX transfer"	
      suite = unittest.TestSuite()
      suite.addTest(a404Test("test_200_VMEREGS_A24"))
      suite.addTest(a404Test("test_210_VMEREGS_A32"))
      suite.addTest(a404Test("test_300_SDRAM_A24"))
      suite.addTest(a404Test("test_305_SDRAM_A24BLK"))
      suite.addTest(a404Test("test_310_SDRAM_A32"))
      suite.addTest(a404Test("test_315_SDRAM_A32BLK"))
      suite.addTest(a404Test("test_320_SDRAM_TX12_DUPLICATION"))
      suite.addTest(a404Test("test_400_LEDS"))
      return suite
      
    def trxCableSuite():	
      "TRX suite: tests with TX/RX cable transfer"	
      suite = unittest.TestSuite()
      suite.addTest(a404Test("test_500_TRXD_BASIC"))
      suite.addTest(a404Test("test_520_TRXD_A24"))
      suite.addTest(a404Test("test_530_TRXD_A32"))
      suite.addTest(a404Test("test_600_TRXC_A24"))
      suite.addTest(a404Test("test_610_TRXC_A32"))
      suite.addTest(a404Test("test_700_IRQ_BASIC"))
      suite.addTest(a404Test("test_710_IRQ_A24"))
      suite.addTest(a404Test("test_720_IRQ_A32"))
      suite.addTest(a404Test("test_800_DISCON"))
      suite.addTest(a404Test("test_810_CRCERR"))
      suite.addTest(a404Test("test_820_RXFIFO"))
      suite.addTest(a404Test("test_900_BENCH"))
      return suite
      
    def trxLoopSuite():	
      "TRX suite: tests with TX/RX loop transfer"	
      suite = unittest.TestSuite()
      suite.addTest(a404Test("test_510_TRXD_BASIC_LOOP"))
      suite.addTest(a404Test("test_525_TRXD_A24_LOOP"))
      suite.addTest(a404Test("test_535_TRXD_A32_LOOP"))
      suite.addTest(a404Test("test_605_TRXC_A24_LOOP"))
      suite.addTest(a404Test("test_705_IRQ_BASIC_LOOP"))
      suite.addTest(a404Test("test_910_BENCH_LOOP"))
      return suite

    def cableSuite():	
      "cable suite: all tests with cable"	
      suite = unittest.TestSuite(( baseSuite(), trxCableSuite() ))
      return suite

    def loopSuite():	
      "cable suite: all tests with HW loop"	
      suite = unittest.TestSuite(( baseSuite(), trxLoopSuite() ))
      return suite
	
    def irqSuite():	
      "IRQ suite: all IRQ tests"	
      suite = unittest.TestSuite()
      suite.addTest(a404Test("test_700_IRQ_BASIC"))
      suite.addTest(a404Test("test_705_IRQ_BASIC_LOOP"))
      suite.addTest(a404Test("test_710_IRQ_A24"))
      suite.addTest(a404Test("test_720_IRQ_A32"))
      return suite	
	
    str =  "===========================================================\n"
    str += "=== A404 FPGA Design Test - %s\n" % date.today()
    str += "=== %FSREV HWARE/01/01a404-/a404_testsw 0.1 2005-10-04%\n"
    str += "===========================================================\n"
    print colorstr(PURPLE,0,str)    
      
    unittest.main()

