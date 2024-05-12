using dnlib.DotNet;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityObserver.Utils;

namespace UnityObserver.Data
{
    internal class GenClass : IGeneratable
    {
        public GenClass(GenNamespace gnamespace, TypeDef type)
        {
            Namespace = gnamespace;
            TypeDef = type;

            Name = type.Name.Replace("<", "").Replace(">", "").Replace(",", "_");
            FilePath = gnamespace.FilePath + "/" + Name + ".h";
        }

        public GenNamespace Namespace { get; }
        public TypeDef TypeDef { get; }
        public string Name { get; }
        public string FilePath { get; }
        public bool IsValueType => TypeDef.IsValueType;
        public bool IsEnum => TypeDef.IsEnum;
        public bool IsClass => TypeDef.IsClass || TypeDef.IsInterface;
        public virtual void Generate(Writer.WriteContext context)
        {
        }
    }
}
