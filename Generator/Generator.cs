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
    internal class Generator
    {
        private Dictionary<string, GenNamespace> _namespaceMap = new Dictionary<string, GenNamespace>();
        public Generator(ModuleDef[] modules)
        {
            // Populate Modules
            Logger.Log(LogType.Info, "Populating Modules...");
            foreach (var module in modules)
            {
                ModuleDefs.Add(module);
            }
            Logger.Log(LogType.Success, $"Found {ModuleDefs.Count} modules.");

            // Populate Namespace and Type Maps
            Logger.Log(LogType.Info, "Populating Namespaces and Types...");
            foreach (var module in ModuleDefs)
            {
                foreach (var type in module.Types)
                {
                    if (!_namespaceMap.ContainsKey(type.Namespace))
                    {
                        _namespaceMap[type.Namespace] = new GenNamespace(this, type.Namespace);
                    }

                    GenClass genClass;
                    if (type.IsClass || type.IsInterface)
                    {
                        genClass = new GenClass(_namespaceMap[type.Namespace], type);
                    }
                    else if (type.IsValueType)
                    {
                        genClass = new GenStruct(_namespaceMap[type.Namespace], type);
                    }
                    else if (type.IsEnum)
                    {
                        genClass = new GenEnum(_namespaceMap[type.Namespace], type);
                    }
                    else
                    {
                        Logger.Log(LogType.Warning, $"Unsupported type: {type.FullName}");
                        continue;
                    }

                    _namespaceMap[type.Namespace].Classes.Add(genClass);
                    TypeMap[type] = genClass;
                }
            }
            Logger.Log(LogType.Success, $"Found {_namespaceMap.Count} namespaces and {TypeMap.Count} types.");
        }
        public void Generate()
        {
            foreach (var ns in _namespaceMap.Values)
            {
                Writer.WriteContext context = Writer.CreateContext(ns.FileName);
                ns.Generate(context);
                context.Write();
            }
        }

        public List<ModuleDef> ModuleDefs { get; } = new List<ModuleDef>();
        public Dictionary<TypeDef, GenClass> TypeMap { get; } = new Dictionary<TypeDef, GenClass>();
    }
}
