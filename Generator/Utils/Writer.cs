using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace UnityObserver.Utils
{
    internal static class Writer
    {
        public static string OutPath { get => Directory.GetCurrentDirectory() + "/Output/"; }
        public static WriteContext CreateContext(string path)
        {
            return new WriteContext(new StringBuilder(), OutPath + path);
        }
        public class WriteContext
        {
            private readonly StringBuilder _builder;
            private string _path;

            public WriteContext(StringBuilder builder, string path)
            {
                _builder = builder;
                _path = path;
            }

            public void Append(string line)
            {
                _builder.AppendLine(line);
            }

            public void Write()
            {
                try
                {
                    File.WriteAllText(_path, _builder.ToString());
                }
                catch (Exception ex)
                {
                    Logger.Log(LogType.Error, $"Failed to write to file: {_path}");
                }
            }
        }
    }
}
