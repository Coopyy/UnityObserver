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
    internal class GenNamespace : IGeneratable
    {
        public GenNamespace(GenModule module, string shortName, GenNamespace parent = null)
        {
            ShortName = shortName;
            Module = module;
            Parent = parent;

            Classes = new List<GenClass>();
            Namespaces = new List<GenNamespace>();

            // Create child namespaces
            foreach (string childNs in GetChildNamespaces())
            {
                GenNamespace ns = new GenNamespace(module, childNs, this);
                Namespaces.Add(ns);
            }

            // Create classes
            foreach (TypeDef type in module.ModuleDef.GetTypes())
            {
                if (type.Namespace == FullName)
                {
                    GenClass genClass;

                    if (type.IsClass || type.IsInterface)
                    {
                        genClass = new GenClass(this, type);
                    }
                    else if (type.IsValueType)
                    {
                        genClass = new GenStruct(this, type);
                    }
                    else if (type.IsEnum)
                    {
                        genClass = new GenEnum(this, type);
                    }
                    else
                    {
                        Logger.Log(LogType.Warning, $"Unsupported type: {type.FullName}");
                        continue;
                    }

                    Classes.Add(genClass);
                    Module.Generator.TypeMap.Add(type, genClass);
                }
            }
        }

        private List<string> GetChildNamespaces()
        {
            HashSet<string> childNamespaces = new HashSet<string>();

            foreach (TypeDef type in Module.ModuleDef.GetTypes())
            {
                string ns = type.Namespace;

                if (ns == ShortName)
                {
                    continue;
                }

                if (ns.StartsWith(FullName + "."))
                {
                    // Find the index of the next dot after the parent namespace
                    int startIndex = FullName.Length + 1;
                    int index = ns.IndexOf('.', startIndex);

                    // Extract the child namespace
                    string childNs = index != -1 ? ns.Substring(startIndex, index - startIndex) : ns.Substring(startIndex);

                    // Add it to the set if not already present
                    childNamespaces.Add(childNs);
                }
            }

            return childNamespaces.ToList();
        }

        public void Generate(Writer.WriteContext context)
        {
            throw new NotImplementedException();
        }

        public string ShortName { get; private set; }
        public string FullName
        {
            get
            {
                GenNamespace current = this;
                string fullName = current.ShortName;

                while (current.Parent != null)
                {
                    current = current.Parent;
                    fullName = current.ShortName + "." + fullName;
                }
                return fullName;
            }
            private set
            {
                ShortName = value;
            }
        }
        public List<GenClass> Classes { get; private set; }
        public List<GenNamespace> Namespaces { get; private set; }
        public GenNamespace Parent { get; private set; }
        public GenModule Module { get; private set; }
    }
}
