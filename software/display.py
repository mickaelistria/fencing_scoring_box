#!/usr/bin/python
import tkinter
import serial
import time
import threading

class MyTkApp(threading.Thread):
   display = None
   def __init__(self):
      threading.Thread.__init__(self)
      self.start()
   def callback(self):
      self.root.quit()
   def run(self):
      self.top = tkinter.Tk()
      self.display = tkinter.Canvas(self.top, height=250, width=950)
      self.reset_lights()
      self.display.pack()
      self.top.mainloop()
   def reset_lights(self):
       global coord1
       coord1 =  10, 10, 200, 200
       global coord2
       coord2 = 210, 10, 400, 200
       global coord3
       coord3 = 410, 10, 600, 200
       global coord4
       coord4 = 610, 10, 800, 200
       self.on_targ_red  = self.display.create_rectangle(coord1, fill="grey")
       self.off_targ_red = self.display.create_rectangle(coord2, fill="grey")
       self.off_targ_grn = self.display.create_rectangle(coord3, fill="grey")
       self.on_targ_grn  = self.display.create_rectangle(coord4, fill="grey")

   def handle_message(self, s):
       if "hitOnTargA  : 1" in s:
           self.on_targ_red  = self.display.create_rectangle(coord1, fill="red")
           # pause timer here
       if "hitOffTargA  : 1" in s:
           self.off_targ_red = self.display.create_rectangle(coord2, fill="yellow")
           # pause timer here
       if "hitOffTargB  : 1" in s:
           self.off_targ_grn = self.display.create_rectangle(coord3, fill="yellow")
           # pause timer here
       if "hitOnTargB  : 1" in s:
           self.on_targ_grn  = self.display.create_rectangle(coord4, fill="green")
           # pause timer here
       if "Reset" in s:
           self.reset_lights()

class Timer:

   def StartTimer(self):
      self.TimerOffset = time.time()
      self.LastTicked = 0
      self.TimeWhenItWasPaused = 0
      self.started = False
      self.paused = False
   
   def Tick(self):
      if self.paused is False:
         NewTicked = time.time() - self.TimerOffset
         diff = NewTicked - self.LastTicked
         self.LastTicked = NewTicked
         return diff
      else:
         print("Cannot Tick, Timer is paused")

   def GetTime(self):
      if self.started is False:
         return 0
      if self.paused is True:
         return self.TimeWhenItWasPaused
      else:
         return time.time() - self.TimerOffset
      
   def Pause(self):
      self.TimeWhenItWasPaused = time.time() - self.TimerOffset
      self.paused = True

   def Unpause(self):
      self.TimerOffset = time.time() - self.TimeWhenItWasPaused
      self.paused = False


#reset_lights()
class serial_coms():
   app = None
   def __init__(self, device, speed, timeout, app):
      self.app = app
      self.ser = serial.Serial(device, speed, timeout=timeout)
      # create new timer here
      print(self.ser.portstr)       # check which port was really used
   def wait_for_command(self):
      if self.app is None or self.app.display is None:
          return
      while 1:
         s = self.ser.readline().decode("utf-8")        # read (timeout)
         if s != '':
            self.app.handle_message(s)
      self.ser.close()

app = MyTkApp()
time.sleep(1) #wait for UI loop to startup
serial_coms = serial_coms('/dev/ttyUSB0', 9600, 0, app)
serial_coms.wait_for_command()
