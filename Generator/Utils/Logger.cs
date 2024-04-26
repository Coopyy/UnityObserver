using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnityObserver.Utils
{
    internal enum LogType
    {
        Info,
        Warning,
        Success,
        Error
    }
    internal class Logger
    {
        public static void Log(LogType type, string message)
        {
            switch (type)
            {
                case LogType.Info:
                    Console.ForegroundColor = ConsoleColor.White;
                    Console.Write("[-] ");
                    break;
                case LogType.Warning:
                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.Write("[x] ");
                    break;
                case LogType.Success:
                    Console.ForegroundColor = ConsoleColor.Green;
                    Console.Write("[+] ");
                    break;
                case LogType.Error:
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.Write("[!] ");
                    break;
            }
            Console.WriteLine(message);
            Console.ResetColor();
        }
    }
}
