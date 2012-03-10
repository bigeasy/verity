/* TODO: These need to log in the directory in which the DLL is installed,
 * right? There or else a temporary directory, the install directory is not
 * writable. Where do you write log files? */
void Log(const TCHAR*, ...);
void WriteLog(const char* message);
void TWriteLog(const TCHAR* message);
