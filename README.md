
**BigConvert**

Use to convert old .big files to .mif files. 
Tested with MIFlib version 0.4,[https://github.com/RadomirVavra/MIFlib](https://github.com/RadomirVavra/MIFlib). 
Coverter us OpenCV library version 4.5.1. (https://opencv.org/opencv-4-5-1/) You must link this library to project. 

Created by Vít Zadina. 

Files to convert is specified in config.txt. Path to files is without .big extension. 

**Config txt structure:**

    distribution: UBO81x81 //inser here distribution type
    filtering: none //insert here filtering type
    F:\vita_skola\convertFiles\MAM2014_016 UBO81x81 //path to file + you can specify distribution of this file
    F:\vita_skola\convertFiles\MAM2014_011
    F:\vita_skola\convertFiles\MAM2014_007
    
**Types of distribution**

 - UBO81x81
 - uniform
 - CoatingRegular
 - CoatingSpecial

**Filtering types**

 - none
 - mipmap //for mipmap texture generation 1,5x size of big file
 - anisotropy //for anisotropy mipmap, 4x size of big file, for anisotropy filtering
 
 Anisotropy and mipmap mif files are for Mitsuba plugin -  [https://github.com/zadinvit/BIGpluginMitsuba](https://github.com/zadinvit/BIGpluginMitsuba)

With older versions of MIFlib you may need link pugiXML.lib file to project. 

> Written with [StackEdit](https://stackedit.io/).