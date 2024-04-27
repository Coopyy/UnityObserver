using dnlib.DotNet;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection.Emit;
using System.Text;
using System.Threading.Tasks;
using UnityObserver.Utils;

namespace UnityObserver.Data
{
    internal class GenNamespace : IGeneratable
    {
        public GenNamespace(Generator generator, string name)
        {
            FullName = name;
            Generator = generator;

            Classes = new List<GenClass>();
        }

        public void Generate(Writer.WriteContext context)
        {
            throw new NotImplementedException();
        }

        public string FullName { get; private set; }
        public string Name => FullName.Split('.').Last();
        public List<GenClass> Classes { get; private set; }
        public Generator Generator { get; private set; }
    }
}
