using dnlib.DotNet;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityObserver.Utils;

namespace UnityObserver.Data
{
    internal class GenEnum : GenClass
    {
        public GenEnum(GenNamespace gnamespace, TypeDef type) : base(gnamespace, type)
        {
        }

        public override void Generate(Writer.WriteContext context)
        {
            base.Generate(TODO);
        }
    }
}
