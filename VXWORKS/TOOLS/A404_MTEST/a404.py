#!/usr/bin/env python
#######################  PEXPECT  -  S k r i p t  ##########################
# 
#       Project: A404 FPGA Design Test
#
#        Author: dieter.pfeuffer@men.de
#         $Date: $
#     $Revision: $
#
#    Description: A404 specific helper routines
#                 
#-------------------------------[ History ]---------------------------------
#
# $Log: test_fpga.py,v $
############################################################################

import sys
import time
import string

import log4py
from log4py import Logger

import log4pyConf
from log4pyConf import log

import common
from common import *

import testparams
from testparams import *

#-----------------------------------
# REGISTERS
#-----------------------------------
# register offsets and bits
TXCTRL = 0x10001
TXCTRL_TXENBL = 0x04
TXCTRL_CRCERR = 0x20

TXCOM = 0x10003

TESTREG = 0x10005
TESTREG_RAMDIS = 0x20

RXCOM = 0x20005

RXCTRL = 0x20001
RXCTRL_LOOP_NO = 0x01 

RXIRQ = 0x20007
RXSTAT_CONNECT = 0x01
RXSTAT_CRCERR = 0x02
RXSTAT_FFFULL_NO = 0x20
RXSTAT_DATALOST = 0x80          

def tInfo(t):
  if t == 'b':
    accInfo = 'bytes'
  elif t == 'w':
   	accInfo = 'words'
  elif t == 'l':
   	accInfo = 'longs'
  return accInfo

class CPU:
  ##### member function #####
  def __init__(self,idStr,ip,ut):
    "constructor"
    self.idStr = idStr
    self.ip = ip
    self.ut = ut

  ##### member function #####
  def open(self):
    log.debug("Open telnet to %s (%s)" % (self.idStr, self.ip))
    self.cons = common.spawnTelnet(self.ip)
    self.cons.expect( shPrompt )

  ##### member function #####
  def close(self):
    log.debug("Close telnet to %s (%s)" % (self.idStr, self.ip))
    self.cons.sendline("logout")
  

class A404:
  ##### member function #####
  def __init__(self,idStr,scarte,a32off,cpu):
    "constructor"
    self.idStr = idStr

    self.scarte = scarte
    self.a32off = a32off
    self.cpu = cpu 

    self.detectBoard()

 	# compatibility mode
    if self.enh == 0:
      # A24 VMEbus address
      self.a24VmeAddr = (scarte & 0xf) << 20      
      # A24/D16 CPU address
      self.a24d16base = a15_A24D16_base + self.a24VmeAddr
	  # A24/D32 CPU address
      self.a24d32base = a15_A24D32_base + self.a24VmeAddr
    
      self.TRX_SIZE = 0x20000
      self.TX1 =      0x00000
      self.TX2 =      0x40000
      self.TX12 =     0x80000
      self.RX1 =      0x20000
      self.RX2 =      0x60000
      self.RXC =      0xD0000
    	
    # enhanced mode
    else :
      # A24 VMEbus address
      self.a24VmeAddr = (scarte & 0xc) << 20      
      # A24/D16 CPU address
      self.a24d16base = a15_A24D16_base + self.a24VmeAddr
	  # A24/D32 CPU address
      self.a24d32base = a15_A24D32_base + self.a24VmeAddr
      
      # A32/D32 VMEbus address
      self.a32VmeAddr = a32off      
	  # A32/D32 CPU address
      self.a32d32base = a15_A32D32_base + self.a32VmeAddr

      self.TRX_SIZE = 0x100000
      self.TX12 =     0x000000
      self.RX1 =      0x100000
      self.RX2 =      0x200000
      self.RXC =      0x3D0000

    self.setA24Base(self.defaultSpace)
    self.vmeAddr = self.a24VmeAddr

  ##### member function #####
  def detectBoard(self):    
    "detect board"

    log.info("detect %s" % self.idStr)
    cmmd = "a404DetectA24(0x%x, 0x%x)" % \
      (a15_A24D16_base, self.scarte)
    ret = execCmdFail(self.cpu.cons, cmmd, -1)

    # CME board?
    if ret == 0:
      self.type = 'OLD_CME'
      self.enh = 0
      self.defaultSpace = 'A24D16'
      self.accTbl = ['b', 'w']
      self.accOpt = 'bw'            
    elif ret == 1:
      self.type = 'A404_COMP_MODE'
      self.enh = 0
      self.defaultSpace = 'A24D32'
      self.accTbl = ['b', 'w', 'l']
      self.accOpt = 'bwl'            
    elif ret == 2:
      self.type = 'A404_ENH_MODE'
      self.enh = 1
      self.defaultSpace = 'A24D32'
      self.accTbl = ['b', 'w', 'l']
      self.accOpt = 'bwl'            
    else :
      self.cpu.ut.fail("*** a404DetectA24 returns unknown value")    

  ##### member function #####
  def setA24Base(self,space):    
    "set address space to use"

    if space == "A24D16":
      self.base = self.a24d16base
    elif space == "A24D32":
      self.base = self.a24d32base
    else :
      self.cpu.ut.fail("*** unknown address space %s" % space)    
            
    print colorstr(YELLOW,0,"%s: %s board-address=0x%x" \
      % (self.idStr,space,self.base)) 
   
  ##### member function #####
  def initBoard(self,hwloop) :   
    "init board"
    if hwloop == 1:
      info = "enabled"
    else:
      info = "disabled"
    	
    log.info("initialize %s (HW-LOOP %s)" % (self.idStr, info))
    cmmd = "a404Init( 0x%x, %d, %d )" % \
      (self.base, self.enh, hwloop)
    execCmdSucc(self.cpu.cons, cmmd, 0)  

  ##### member function #####
  def enableA32(self):
    "set and enable A32 space"
    cmmd = "a404EnableA32(0x%x, 0x%x)" % (self.base, a404_1_a32off)
    execCmdSucc(self.cpu.cons, cmmd, 0)
    self.baseOld = self.base
    self.base = self.a32d32base
    self.vmeAddr = self.a32VmeAddr
    print colorstr(YELLOW,0,"%s: A32 board-address=0x%x" \
      % (self.idStr,self.base)) 

  ##### member function #####
  def disableA32(self):
    "disable A32 space"
    cmmd = "a404DisableA32(0x%x)" % self.base
    execCmdSucc(self.cpu.cons, cmmd, 0)
    self.base = self.baseOld
    self.vmeAddr = self.a24VmeAddr
    print colorstr(YELLOW,0,"%s: A24 board-address=0x%x" \
      % (self.idStr,self.base)) 

  ##### member function #####   
  def errIfNotBrd(self,brd):
    "error if not the specified board"
    if self.type != brd :
      self.cpu.ut.fail("*** board %s must be %s" % \
        (self.idStr, brd))  

  ##### member function #####   
  def skipIfNotBrd(self,brd,task):
    "skip task if not the specified board"
    if self.type != brd :
      log.info("%s skipped for board %s=%s" % \
        (task, self.idStr, self.type))
      return 1
    return 0

  ##### member function #####   
  def skipIfBrd(self,brd,task):
    "skip task if the specified board"
    if self.type == brd :
      log.info("%s skipped for board %s=%s" % \
        (task, self.idStr, self.type))
      return 1
    return 0
          
  ##### member function #####
  def txn(self,ch,n,t) :
    "write n elements to tx space"

    if ch == 'tx1' :	# not for enhanced mode!
      txStart = self.TX1
      mtestOpt = ""  
    elif ch == 'tx2' :	# not for enhanced mode!
      txStart = self.TX2
      mtestOpt = ""  
    elif ch == 'tx12' :
      txStart = self.TX12
      mtestOpt = "-w"  
    else:
      self.cpu.ut.fail("*** illegal ch=%d" % ch)     

    start = self.base + txStart
    end = start + self.TRX_SIZE - 1
 
    if n != 0xffffffff:
      end = start + (n * 2)
      log.info("write %d %s to %s space" % (n, tInfo(t), ch) )  
    else:
      log.info("write %s to entire %s space" % (tInfo(t), ch) )   
    
    cmmd = "mtest \"0x%x 0x%x -o=2 -q=%d -t=%s %s\"" % \
      (start, end, mtestMaxErr, t, mtestOpt)
    execCmdSucc(self.cpu.cons, cmmd, 0)               

  ##### member function #####
  def tx(self,ch,t) :
    "write to entire tx space"
    self.txn( ch, 0xffffffff, t )

  ##### member function #####
  def txBlk(self,mod) :
    "blk write to tx space"

    start = self.vmeAddr + self.TX12
    end = start + self.TRX_SIZE - 1
 
    log.info("blk write %s to entire TX12 space" % (mod) )   
    cmmd = "mtest \"0x%x 0x%x -o=2 -q=%d -t=w -m=%s,10\"" % \
      (start, end, mtestMaxErr, mod)
    execCmdSucc(self.cpu.cons, cmmd, 0)               
         
  ##### member function #####
  def rx(self,ch,t) :
    "read from rx space"
    self.rx_clr( ch, t, 0 )
  
  ##### member function #####
  def clrRx(self,ch) :
    "clear rx space" 
    self.rx_clr( ch, 'w', 1 )  
          
  ##### member function #####
  def rx_clr(self,ch,t,clear) :
    "read/clear rx space"

    if ch == 'rx1' :
      rxStart = self.RX1
    elif ch == 'rx2' :
      rxStart = self.RX2
    elif ch == 'tx1' :	# not for enhanced mode!
      rxStart = self.TX1
    elif ch == 'tx2' :	# not for enhanced mode!
      rxStart = self.TX2
    elif ch == 'tx12' :
      rxStart = self.TX12
    else:
      self.cpu.ut.fail("*** illegal ch=%d" % ch)       
    
    start = self.base + rxStart
    size = self.TRX_SIZE
    end = start + size-1
    
    if clear :
      cmmd = "memset( 0x%x, 0x00, 0x%x )" % \
        (start, size)
      log.info("clear data in %s space" % \
        ch )
      execCmdFail(self.cpu.cons, cmmd, 0)    
    else:         
      cmmd = "mtest \"0x%x 0x%x -o=2 -q=%d -t=%s -v\"" % \
        (start, end, mtestMaxErr, t)
      log.info("read and verify %s from %s space" % \
        (tInfo(t), ch) )
      execCmdSucc(self.cpu.cons, cmmd, 0)               

  ##### member function #####
  def rxBlk(self,ch,mod) :
    "blk read rx space"

    if ch == 'rx1' :
      rxStart = self.RX1
    elif ch == 'rx2' :
      rxStart = self.RX2
    else:
      self.cpu.ut.fail("*** illegal ch=%d" % ch)   

    start = self.vmeAddr + rxStart
    end = start + self.TRX_SIZE - 1
 
    log.info("blk read %s from entire %s space" % (mod,ch) )   
    cmmd = "mtest \"0x%x 0x%x -o=2 -q=%d -t=v -m=%s,10 -v\"" % \
      (start, end, mtestMaxErr, mod)
    execCmdSucc(self.cpu.cons, cmmd, 0)     

  ##### member function #####
  def checkClrRxstat12(self,expect) :          
    "check and clear RXSTAT1/RXSTAT2"
    log.info("check and clear RXSTAT1/RXSTAT2" )
  
    for ch in [1,2]:
      cmmd = "a404RxstatGetClr( 0x%x, %d, %d )" % \
        (self.base, self.enh, ch)  
      execCmdSucc(self.cpu.cons, cmmd, expect)
                   
  ##### member function #####
  def checkClrRxstat(self,ch,expect) :          
    "check and clear RXSTAT"
    log.info("check and clear RXSTAT%d" % ch )
  
    cmmd = "a404RxstatGetClr( 0x%x, %d, %d )" % \
      (self.base, self.enh, ch)  
    execCmdSucc(self.cpu.cons, cmmd, expect)
  
  ##### member function #####
  def getClrRxstat(self,ch) :          
    "get and clear RXSTAT"
    log.info("get and clear RXSTAT%d" % ch )
  
    cmmd = "a404RxstatGetClr( 0x%x, %d, %d )" % \
      (self.base, self.enh, ch)  
    return getShellVarValue( self.cpu.cons, cmmd )
  
  ##### member function #####
  def checkClrTxstat(self,ch,expect) :          
    "check and clear TXSTAT"
    log.info("check and clear TXSTAT%d" % ch )
  
    cmmd = "a404TxstatGetClr( 0x%x, %d, %d )" % \
      (self.base, self.enh, ch)  
    execCmdSucc(self.cpu.cons, cmmd, expect)     
    
  ##### member function #####
  def clrTxstat(self,ch) :          
    "clear TXSTAT"
    log.info("clear TXSTAT%d" % ch )
  
    cmmd = "a404TxstatGetClr( 0x%x, %d, %d )" % \
      (self.base, self.enh, ch)  
    getShellVarValue( self.cpu.cons, cmmd )        
            
  ##### member function #####
  def sendCmd(self,ch,cmdByte) :            
    "send command"
    cmmd = "a404ByteRegWrite( 0x%x, %d, %d, 0x%x, 0x%x )" % \
      (self.base, self.enh, ch, TXCOM, cmdByte)  
    execCmdSucc(self.cpu.cons, cmmd, 0)
  
  ##### member function #####
  def recvVeriCmd(self,ch,expByte) :           
    "receive and verify command"
    cmmd = "a404ByteRegRead( 0x%x, %d, %d, 0x%x )" % \
      (self.base, self.enh, ch, RXCOM)  
    rcvByte = getShellVarValue( self.cpu.cons, cmmd )

    # verify command
    if rcvByte != expByte:
      self.cpu.ut.fail("*** rcvByte=0x%x but expByte=0x%x" % \
      (rcvByte, expByte))
            
  ##### member function #####
  def enableHwLoop(self) :            
    "enable HW loop-back"
    log.info("enable TX1->RX1, TX2->RX2 HW loop (disable external lanes)")
    for ch in [1,2]:
      cmmd = "a404ByteRegClrmask( 0x%x, %d, %d, 0x%x, 0x%x )" % \
        (self.base, self.enh, ch, RXCTRL, RXCTRL_LOOP_NO)  
      execCmdSucc(self.cpu.cons, cmmd, 0)

  ##### member function #####
  def disableHwLoop(self) :            
    "disable HW loop-back"
    log.info("disable TX1->RX1, TX2->RX2 HW loop (enable external lanes)")
    for ch in [1,2]:
      cmmd = "a404ByteRegSetmask( 0x%x, %d, %d, 0x%x, 0x%x )" % \
        (self.base, self.enh, ch, RXCTRL, RXCTRL_LOOP_NO)  
      execCmdSucc(self.cpu.cons, cmmd, 0)
 
  ##### member function #####
  def enableTX1TX2(self) :            
    "enable TX1/TX2 transmitter"
    log.info("enable TX1/TX2 transmitter")
    for ch in [1,2]:
      cmmd = "a404ByteRegClrmask( 0x%x, %d, %d, 0x%x, 0x%x )" % \
        (self.base, self.enh, ch, TXCTRL, TXCTRL_TXENBL)  
      execCmdSucc(self.cpu.cons, cmmd, 0)

  ##### member function #####
  def disableTX1TX2(self) :            
    "disable TX1/TX2 transmitter"
    log.info("disable TX1/TX2 transmitter")
    for ch in [1,2]:
      cmmd = "a404ByteRegSetmask( 0x%x, %d, %d, 0x%x, 0x%x )" % \
        (self.base, self.enh, ch, TXCTRL, TXCTRL_TXENBL)  
      execCmdSucc(self.cpu.cons, cmmd, 0)  