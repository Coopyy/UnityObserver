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
            GenModule module = new GenModule(ModuleDefMD.Load(@"F:\Steam\steamapps\common\Unturned\Unturned_Data\Managed\mscorlib.dll", ModuleDef.CreateModuleContext()));

            foreach (GenNamespace genNamespace in module.BaseNamespaces)
            {
                int indent = 0;
                PrintNamespaces(genNamespace, indent);
            }

            Console.ReadLine();
        }

        static void PrintNamespaces(GenNamespace genNamespace, int indent)
        {
            Console.WriteLine(new string(' ', indent * 2) + genNamespace.FullName);

            foreach (var childNamespace in genNamespace.Namespaces)
            {
                PrintNamespaces(childNamespace, indent + 1);
            }
        }
    }
}