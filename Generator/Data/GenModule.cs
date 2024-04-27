﻿using dnlib.DotNet;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityObserver.Utils;

namespace UnityObserver.Data
{
    internal class GenModule : IGeneratable
    {
        public GenModule(Generator generator, ModuleDef moduleDef)
        {
            ModuleDef = moduleDef;
            Generator = generator;

            BaseNamespaces = new List<GenNamespace>();

            // Get the base namespaces
            List<string> baseNamespaces = GetBaseNamespaces();

            // Create a GenNamespace for each base namespace
            foreach (string baseNs in baseNamespaces)
            {
                GenNamespace ns = new GenNamespace(this, baseNs);
                BaseNamespaces.Add(ns);
            }
        }

        private List<string> GetBaseNamespaces()
        {
            HashSet<string> baseNamespaces = new HashSet<string>();
            foreach (var type in ModuleDef.Types)
            {
                // Get the namespace string
                string ns = type.Namespace.String;

                // Find the first segment of the namespace
                int dotIndex = ns.IndexOf('.');
                string baseNs = dotIndex != -1 ? ns.Substring(0, dotIndex) : ns;

                // Add it to the set if not already present
                baseNamespaces.Add(baseNs);
            }
            // Convert the set to a list and return
            return baseNamespaces.ToList();
        }

        public void Generate(Writer.WriteContext context)
        {
            throw new NotImplementedException();
        }

        public ModuleDef ModuleDef { get; private set; }
        public Generator Generator { get; private set; }
        public List<GenNamespace> BaseNamespaces { get; private set; }
    }
}
