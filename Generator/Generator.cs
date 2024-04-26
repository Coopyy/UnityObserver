using dnlib.DotNet;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityObserver.Data;
using UnityObserver.Utils;

namespace UnityObserver
{
    internal class Generator : IGeneratable
    {
        public Generator(ModuleDef[] modules)
        {
            foreach (var module in modules)
            {
                Logger.Log(LogType.Success, $"Found Module: {module.Name}");
                GenModules.Add(new GenModule(this, module));
            }
        }
        public void Generate()
        {

        }

        public List<GenModule> GenModules { get; } = new List<GenModule>();
        public Dictionary<TypeDef, GenClass> TypeMap { get; } = new Dictionary<TypeDef, GenClass>();
    }
}
