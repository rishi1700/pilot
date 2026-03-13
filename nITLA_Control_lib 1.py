import serial
import time
import pandas as pd
import re
import numpy as np
import sys

'''
    Loads a look-up table (LUT) with setpoints
    str fpath: Path to the LUT as a .csv
    Returns a list of valid target frequencies and the corresponding operating currents/voltages
    f_lst: Set frequencies (THz),
    w_lst: Set wavelength (nm)
    V1_lst: R1 voltages (V), 
    V2_lst: R2 voltages (V), 
    V3_lst: Phase voltages (V),
    Ig_lst: Gain currents (mA),
    Is_lst: SOA currents (mA)
    PD1_lst: PD1 ADC values
    PD2_lst: PD2 ADC values
    PD3_lst: PD3 ADC values
    VTEC_lst: TEC voltages (V)
'''
class LookUpTable:
    def __init__(self, fpath):
        data = pd.read_csv(fpath)
        f_set = data[data.keys()[0]]
        f_real = data[data.keys()[1]]
        w_real = data[data.keys()[2]]

        #Loads the target columns
        V1_lst = data[data.keys()[3]]
        V2_lst = data[data.keys()[4]]
        V3_lst = data[data.keys()[5]]
        Ig_lst = data[data.keys()[6]]
        Is_lst = data[data.keys()[7]]
        PD1_lst = data[data.keys()[10]]
        PD2_lst = data[data.keys()[11]]
        PD3_lst = data[data.keys()[12]]
        VTEC_lst = data[data.keys()[13]]

        #Filters out rows invalid points
        self.f_lst = f_set[f_real != 0.].to_list()
        self.w_lst = w_real[f_real != 0.].to_list()
        self.V1_lst = V1_lst[f_real != 0.].to_list()
        self.V2_lst = V2_lst[f_real != 0.].to_list()
        self.V3_lst = V3_lst[f_real != 0.].to_list()
        self.Ig_lst = Ig_lst[f_real != 0.].to_list()
        self.Is_lst = Is_lst[f_real != 0.].to_list()
        self.PD1_lst = PD1_lst[f_real != 0.].to_list()
        self.PD2_lst = PD2_lst[f_real != 0.].to_list()
        self.PD3_lst = PD3_lst[f_real != 0.].to_list()
        self.VTEC_lst = VTEC_lst[f_real != 0.].to_list()

        try:
            validated_lst = data[data.keys()[23]]
            self.validated_lst = validated_lst[f_real != 0.].to_list()
        except:
            self.validated_lst = None


'''
A control class for interfacing with the nITLA
str COM: COM port of the USB interface
str fpath: Path to the LUT as a .csv
'''
class nITLA:

    def __init__(self, COM="COM3", fpath="LUTs/Unit5_Boxed_CenterMode_T20_Ta20_Ig120_FullLUT.csv"):
        self.ser = serial.Serial(
            port=COM,
            baudrate=115200,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=1
        )
        if not self.ser.is_open:
            self.ser.open()

        self.LUT = LookUpTable(fpath)

        #########################################################################################
        #ASCII Commands to initialise unit
        #########################################################################################

        command = f"ENRV#"
        try:
            self.ser.write(command.encode('utf-8'))
            self.flush()
        except Exception as e:
            print(f"Error: {e}")
            self.ser.close()

        command = f"ENNV#"
        try:
            self.ser.write(command.encode('utf-8'))
            self.flush()
        except Exception as e:
            print(f"Error: {e}")
            self.ser.close()

        #########################################################################################


    # Converts input current to raw DAC values for Gain section
    def float_to_dec_gain(self, number):
        count = int((25.779 * number + 168.93))
        #count = int((25.779 * number + 168.93) - 168)
        return count

    # Converts input current to raw DAC values for SOA section
    def float_to_dec_soa(self, number):
        count = int(3.0225 * number - 16.605)
        if count < 0:
            count = 0
        return count

    # Converts input voltage to raw DAC values for tuners
    def float_to_dec_tuner(self, number):
        count = int(161.51 * number - 5.1112)
        if count < 0:
            count = 0
        return count


    '''
        Set the current of the Gain or SOA section
        float I: Input current in mA
        str sec: G for Gain section, S for SOA section
    '''
    def set_current(self, I, sec="G"):
        if sec == "G":
            value = self.float_to_dec_gain(I)
            command = f"GAIN#{value}\n"             #ASCII command to set Gain current
        elif sec == "S":
            value = self.float_to_dec_soa(I)
            command = f"SOAI#{value}\n"             #ASCII command to set SOA current

        else:
            print("Invalid selection: (G, S)")

        try:
            self.ser.write(command.encode('utf-8'))
            self.flush()
        except Exception as e:
            print(f"Error: {e}")
            self.ser.close()

    def set_TEC(self, T):
        #value = self.float_to_dec_gain(T)
        command = f"STMP#{T}\n"                     #ASCII command to set TEC

        try:
            self.ser.write(command.encode('utf-8'))
            self.flush()
        except Exception as e:
            print(f"Error: {e}")
            self.ser.close()

    '''
        Set the voltage to the tuning sections
        float V: Input reverse bias voltage in V (Positive value)
        str sec: R1 for Ring 1, R2 for Ring 2, P for Phase
    '''
    def set_tuner_voltage(self, V, sec="R1"):
        V = abs(V)
        V = self.float_to_dec_tuner(V)
        print(V)
        if sec == "R1":
            command = f"SR1V#{V}\n"                     #ASCII command to set V1
        elif sec == "R2":
            command = f"SR2V#{V}\n"                     #ASCII command to set V2
        elif sec == "P":
            command = f"PHSE#{V}\n"                     #ASCII command to set Phase

        else:
            print("Invalid selection: (R1, R2, P)")

        try:
            self.ser.write(command.encode('utf-8'))
            self.flush()
        except Exception as e:
            print(f"Error: {e}")
            self.ser.close()

    def blank_V(self):
        command = "AT#"                                     #ASCII command to blank tuners
        try:
            self.ser.write(command.encode('utf-8'))
            self.flush()
        except Exception as e:
            print(f"Error: {e}")
            self.ser.close()

    '''
    Set output frequency from LUT (Blanks gain between set points)
    int tab_index: index of the (valid) look-up table row
    bool blank: If Gain current is zeroed to prevent hysteresis
    Returns set frequency (THz) and wavelength (nm)
    '''
    def set_frequency(self, tab_index, blank=True, VZero=False):
        self.set_tuner_voltage(self.LUT.V1_lst[tab_index], "R1")
        self.set_tuner_voltage(self.LUT.V2_lst[tab_index], "R2")
        self.set_tuner_voltage(self.LUT.V3_lst[tab_index], "P")
        self.set_current(self.LUT.Is_lst[tab_index], "S")
        self.set_current(self.LUT.Ig_lst[tab_index], "G")

        if VZero:
            self.blank_V()

        if blank:
            self.set_current(0, "G")
            self.set_current(self.LUT.Ig_lst[tab_index], "G")

    def read_feedback(self):
        command = f"RTMP#"                                          #ASCII command to read T, VTEC and all 3 PDs \\TODO should be seperate functions
        try:
            self.ser.write(command.encode('utf-8'))
            time.sleep(0.1)
            response = self.ser.read_all().decode('utf-8')
            out = re.split(",", response)
            #print(out)
            MPD = int(out[0])
            WLPD = int(out[1])
            WMPD = int(out[2])
            Traw = int(out[3])
            Rh = 10000 / ((2.52 / (float(Traw)/1000)) - 1)
            Th = (-22.57 * np.log(Rh)) + 233.42

            try:
                TMC = float(out[4])
                VTEC = float(out[5])
                return round(Th, 2), MPD, WLPD, WMPD, TMC, VTEC
            except:
                return round(Th, 2), MPD, WLPD, WMPD

        except Exception as e:
            print(f"Error: {e}")
            self.ser.close()

    '''
        Set all bias currents/voltages to 0 and close the serial port
    '''
    def shutdown(self):
        self.set_current(0, "G")
        self.set_current(0, "S")
        self.set_tuner_voltage(0, "R1")
        self.set_tuner_voltage(0, "R2")
        self.set_tuner_voltage(0, "P")
        self.close()

    #Flush the buffer after a write command
    def flush(self):
        time.sleep(0.1)
        self.ser.read_all().decode('utf-8')

    #Close the serial port
    def close(self):
        self.ser.close()
