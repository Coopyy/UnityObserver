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
            context.Append($"#ifndef {FullName.ToUpper()}_H");
            context.Append($"#define {FullName.ToUpper()}_H");
            context.NewLine();

            foreach (var genClass in Classes)
            {
                context.Append($"#include \"{genClass.FilePath}\"");

                Writer.WriteContext classContext = Writer.CreateContext(genClass.FilePath);
                genClass.Generate(classContext);
                classContext.Write();
            }

            context.NewLine();
            context.Append("#endif");
        }

        public string FullName { get; private set; }
        public string Name => FullName.Split('.').Last();
        public string FilePath => String.IsNullOrEmpty(FullName) ? "-" : FullName.Replace('.', '/');
        public string FileName => FilePath.Replace('.', '_') + ".h";
        public List<GenClass> Classes { get; private set; }
        public Generator Generator { get; private set; }
    }
}
