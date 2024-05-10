using dnlib.DotNet;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityObserver.Data;

namespace UnityObserver
{
    internal class Program
    {
        static void Main(string[] args)
        {
            // Loop files in directory
            List<ModuleDef> modules = new List<ModuleDef>();
            foreach (var file in System.IO.Directory.GetFiles(@"F:\Steam\steamapps\common\Unturned\Unturned_Data\Managed\", "*.dll"))
            {
                // Load module
                ModuleDef moduleDef = ModuleDefMD.Load(file, ModuleDef.CreateModuleContext());
                modules.Add(moduleDef);
            }

            Generator generator = new Generator(modules.ToArray());
            generator.Generate();

            Console.ReadLine();
        }
    }
}