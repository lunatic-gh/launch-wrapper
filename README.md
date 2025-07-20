# Launch-Wrapper

#### This tool was mainly designed to be able to launch non-exe files on Windows without using batch files, since that forces a commandline-window.

### Syntax:
- Example to open a jar file with a specific qt platform path:
  - path\to\launch-wrapper.exe QT_QPA_PLATFORM_PLUGIN_PATH=.\bin\platform -- javaw -jar your-app.jar
 
### My antivirus says this is a trojan
- So does mine. The Program executes shell commands (duh), which looks suspicious to most antivirus softwares. In this case it's a false-positive, you can check the cpp file yourself, it's not THAT big.
