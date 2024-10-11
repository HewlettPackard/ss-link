# Run the following commands to install the libraries and dependencies
# sudo apt install python3-pip
# sudo pip install pandas
# sudo pip install openpyxl
# To run this script
# python3 cable_list_parser_and_db_creator.py
# The script will parse the excel sheet containing all the supported cables.
# The excel sheet needs to be in the same folder as this script and must be named "SlingshotCableLookup_08082023.xlsm"
# The Sheet with cable list inside the excel file must be named "S1 S2 Cable List'
# The script will create a header file called "sl_media_data_cable_db.h" which will contain a DB with all supported cables

import pandas as pd
import openpyxl
import re


#create a header file and open it for writing
file1 = open("sl_media_data_cable_db.h", "w") 

# Define variable to load the dataframe
dataframe = openpyxl.load_workbook("SlingshotCableCompatibilityMatrix_v1.0.xlsm")
 
# Define variable to read sheet
#dataframe1 = dataframe.active
dataframe1 = dataframe['S1 S2 Cable List']

#find the numbe of rows in excel sheet
curr_row = 1
counter = 1
while (1):
    cell_obj = dataframe1.cell(row = curr_row+1, column = 1)
    alpha_num_pn = str(cell_obj.value)
    if alpha_num_pn == "None":
        break
    else:
        counter = counter + 1
    curr_row = curr_row + 1
        

HP_PN = [] #array of PNs which will be sorted later
for row in range(1, counter):
    curr_row = row
    cell_obj = dataframe1.cell(row = curr_row+1, column = 7)
    length = str(cell_obj.value)
    if length == "HPE Slingshot L1 1x16 Sw Cbl Kit Cray EX" or length == "HPE Slingshot L1 2x16 Sw Cbl Kit Cray EX" or length == "HPE Slingshot L1 1x32 Sw Cbl Kit Cray EX":
        continue #ignore the cable kits
    cell_obj = dataframe1.cell(row = curr_row+1, column = 1)
    #read the alphanumeric PNs
    alpha_num_pn = str(cell_obj.value)
    #convert alpha numeric PNs to numeric PNs
    hp_pn_str = re.sub(r'[^0-9]', '', alpha_num_pn)
    HP_PN.append(int(hp_pn_str))

#sort according to PNs
HP_PN.sort()
total_entries = str(len(HP_PN))

file1.write("/* SPDX-License-Identifier: GPL-2.0 */\n")
file1.write("/* Copyright 2023,2024 Hewlett Packard Enterprise Development LP */\n\n")
file1.write("/* This file is auto-generated through script and should not be modified */\n\n")
file1.write("#ifndef _SL_MEDIA_DATA_CABLE_DB_H_\n")
file1.write("#define _SL_MEDIA_DATA_CABLE_DB_H_\n\n")
file1.write("#include \"sl_media_jack.h\"\n")
file1.write("\nstatic struct sl_media_cable_attr cable_db[] = {\n")

#Algorithm: We take a PN from sorted list and for each PN, we loop through the excel sheet trying to find its associated data.
#Once we find it, we print that data and then delete it from the excel sheet to avoid overwriting (This is important since there are different cables with same PNs)
for i in range(len(HP_PN)):
    curr_row = 1
    while(1):
        cell_obj = dataframe1.cell(row = curr_row+1, column = 7)
        length = str(cell_obj.value)
        if length == "HPE Slingshot L1 1x16 Sw Cbl Kit Cray EX" or length == "HPE Slingshot L1 2x16 Sw Cbl Kit Cray EX" or length == "HPE Slingshot L1 1x32 Sw Cbl Kit Cray EX":
            curr_row = curr_row + 1
            continue
        cell_obj = dataframe1.cell(row = curr_row+1, column = 1)
        alpha_num_pn = str(cell_obj.value) #read the alpha numeric PN
        #convert alpha numeric PNs to numeric PNs
        hp_pn_str = re.sub(r'[^0-9]', '', alpha_num_pn)
        hp_pn_int = int(hp_pn_str)
        if HP_PN[i] == hp_pn_int:
            file1.write("\t{\n")
            file1.write("\t\t.hpe_pn                 = " + str(HP_PN[i]) + ",\n")
            
            cell_obj = dataframe1.cell(row = curr_row+1, column = 3) #read the vendor
            if str(cell_obj.value) == "TE":
                file1.write("\t\t.vendor                 = " + "SL_MEDIA_VENDOR_TE" + ",\n")
            elif str(cell_obj.value) == "Bizlink" or str(cell_obj.value) == "BizLink":
                file1.write("\t\t.vendor                 = " + "SL_MEDIA_VENDOR_BIZLINK" + ",\n")
            elif str(cell_obj.value) == "Hisense":
                file1.write("\t\t.vendor                 = " + "SL_MEDIA_VENDOR_HISENSE" + ",\n")
            elif str(cell_obj.value) == "Coherent (Finisar II-VI)":
                file1.write("\t\t.vendor                 = " + "SL_MEDIA_VENDOR_FINISAR" + ",\n")
            elif str(cell_obj.value) == "Cloud Light":
                file1.write("\t\t.vendor                 = " + "SL_MEDIA_VENDOR_CLOUD_LIGHT" + ",\n")
            elif str(cell_obj.value) == "Molex":
                file1.write("\t\t.vendor                 = " + "SL_MEDIA_VENDOR_MOLEX" + ",\n")
            tmp_vendor = str(cell_obj.value)

            cell_obj = dataframe1.cell(row = curr_row+1, column = 6) #read the type
            if str(cell_obj.value) == "DAC" or str(cell_obj.value) == "PEC":
                file1.write("\t\t.type                   = " + "SL_MEDIA_TYPE_PEC" + ",\n")
            elif str(cell_obj.value) == "AOC-A" or str(cell_obj.value) == "AOC-D" or str(cell_obj.value) == "AOC":
                file1.write("\t\t.type                   = " + "SL_MEDIA_TYPE_AOC" + ",\n")
            elif str(cell_obj.value) == "ACC":
                file1.write("\t\t.type                   = " + "SL_MEDIA_TYPE_ACC" + ",\n")
            elif str(cell_obj.value) == "AEC":
                file1.write("\t\t.type                   = " + "SL_MEDIA_TYPE_AEC" + ",\n")
            tmp_type = str(cell_obj.value)

            cell_obj = dataframe1.cell(row = curr_row+1, column = 7) #read the length
            length = str(cell_obj.value)
            #num_length = length.rstrip(length[-1]) #remove whitespaces
            whitelist = set('0123456789.')
            num_length = ''.join(filter(whitelist.__contains__, length)) #remove whitespaces and letters
            num_length = float(num_length) #convert string to float
            num_length = num_length * 100 # multiply by 100 to convert meter to cm
            num_length = int(num_length)  #convert float to int
            num_length = str(num_length) #convert int to string
            file1.write("\t\t.length_cm              = " + num_length + ",\n")
            tmp_length = num_length;

            cell_obj = dataframe1.cell(row = curr_row+1, column = 8) #read the speed
            if(str(cell_obj.value) == "200G E"):
                file1.write("\t\t.max_speed              = " + "SL_MEDIA_SPEEDS_SUPPORT_CK_200G" + ",\n")
            elif(str(cell_obj.value) == "400G E"):
                file1.write("\t\t.max_speed              = " + "SL_MEDIA_SPEEDS_SUPPORT_CK_400G" + ",\n")
            elif(str(cell_obj.value) == "800Gb"):
                file1.write("\t\t.max_speed              = " + "SL_MEDIA_SPEEDS_SUPPORT_CK_800G" + ",\n")
            tmp_max_speed = str(cell_obj.value)

            cell_obj = dataframe1.cell(row = curr_row+1, column = 6) #read the type
            if str(cell_obj.value) == "DAC" or str(cell_obj.value) == "PEC":
                file1.write("\t\t.serdes_settings.pre1   = " + "0" + ",\n")
                file1.write("\t\t.serdes_settings.pre2   = " + "0" + ",\n")
                file1.write("\t\t.serdes_settings.pre3   = " + "0" + ",\n")
                file1.write("\t\t.serdes_settings.cursor = " + "100" + ",\n")
                file1.write("\t\t.serdes_settings.post1  = " + "0" + ",\n")
                file1.write("\t\t.serdes_settings.post2  = " + "0" + ",\n")
            else:
                file1.write("\t\t.serdes_settings.pre1   = " + "-20" + ",\n")
                file1.write("\t\t.serdes_settings.pre2   = " + "0" + ",\n")
                file1.write("\t\t.serdes_settings.pre3   = " + "0" + ",\n")
                file1.write("\t\t.serdes_settings.cursor = " + "116" + ",\n")
                file1.write("\t\t.serdes_settings.post1  = " + "0" + ",\n")
                file1.write("\t\t.serdes_settings.post2  = " + "0" + ",\n")

            file1.write("\t},\n")

            sheet = dataframe['S1 S2 Cable List']
            sheet.delete_rows(curr_row+1, 1)
            break
            
        curr_row = curr_row + 1


file1.write("};\n\n")
file1.write("#endif /* _SL_MEDIA_DATA_CABLE_DB_H_ */")
file1.close()
