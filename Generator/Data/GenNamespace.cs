using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityObserver.Utils;

namespace UnityObserver.Data
{
    internal class GenNamespace : IGeneratable
    {
        public GenNamespace(Generator generator, string fullName)
        {
            FullName = String.IsNullOrEmpty(fullName) ? "-" : fullName;
            FilePath = FullName.Replace('.', '/');
            FileName = FilePath.Replace('/', '_') + ".h";
            Name = FullName.Substring(FullName.LastIndexOf('.') + 1);

            Generator = generator;
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

        public string FullName { get; }
        public string Name { get; }
        public string FilePath { get; }
        public string FileName { get; }
        public List<GenClass> Classes { get; } = new List<GenClass>();
        public Generator Generator { get; }
    }
}
