using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityObserver.Utils;

namespace UnityObserver
{
    internal interface IGeneratable
    {
        void Generate(Writer.WriteContext context = null);
    }
}
