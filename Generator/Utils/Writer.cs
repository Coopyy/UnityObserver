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
        private static Dictionary<string, WriteContext> _contexts = new Dictionary<string, WriteContext>();
        private static bool _initialized = false;
        public static WriteContext CreateContext(string path)
        {
            if (!_initialized)
            {
                if (Directory.Exists(OutPath))
                    Directory.Delete(OutPath, true);

                Directory.CreateDirectory(OutPath);

                _initialized = true;
            }

            if (_contexts.ContainsKey(path))
            {
                return _contexts[path];
            }

            if (path.Contains("/"))
                Directory.CreateDirectory(OutPath + Path.GetDirectoryName(path));

            var builder = new StringBuilder();
            var context = new WriteContext(builder, OutPath + path);
            _contexts.Add(path, context);
            return context;
        }
        public class WriteContext
        {
            private readonly StringBuilder _builder;
            private readonly string _path;

            public WriteContext(StringBuilder builder, string path)
            {
                _builder = builder;
                _path = path;
            }

            public void Append(string line)
            {
                _builder.AppendLine(line);
            }

            public void NewLine()
            {
                _builder.AppendLine();
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
                    Logger.Log(LogType.Error, ex.Message);
                }
            }
        }
        public static string OutPath { get => Directory.GetCurrentDirectory() + "/Output/"; }
    }
}
