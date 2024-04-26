using dnlib.DotNet;
using System;
using System.Collections.Generic;
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
        }

        public GenNamespace Namespace { get; }
        public TypeDef TypeDef { get; }
        public string Name => TypeDef.Name;
        public bool IsValueType => TypeDef.IsValueType;
        public bool IsEnum => TypeDef.IsEnum;
        public bool IsClass => TypeDef.IsClass || TypeDef.IsInterface;
        public virtual void Generate()
        {
            throw new NotImplementedException();
        }
    }
}
