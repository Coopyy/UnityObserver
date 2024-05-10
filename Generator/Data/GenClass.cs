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
        private readonly string _name;
        public GenClass(GenNamespace gnamespace, TypeDef type)
        {
            Namespace = gnamespace;
            TypeDef = type;

            _name = type.Name.Replace("<", "").Replace(">", "").Replace(",", "_");
        }

        public GenNamespace Namespace { get; }
        public TypeDef TypeDef { get; }
        public string Name => _name;
        public bool IsValueType => TypeDef.IsValueType;
        public bool IsEnum => TypeDef.IsEnum;
        public bool IsClass => TypeDef.IsClass || TypeDef.IsInterface;
        public string FilePath => Namespace.FilePath + "/" + Name + ".h";
        public virtual void Generate(Writer.WriteContext context)
        {
        }
    }
}
