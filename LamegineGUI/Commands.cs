using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LamegineGUI
{
    namespace Commands
    {
        public class SetModelPath
        {
            public string action = "set_model_path";
            public string value;
            public SetModelPath(string path)
            {
                value = path;
            }
        }

        public class Command
        {
            private static int cmdCounter = 2;
            public int id = 0;
            public object data = null;
            public Command(object obj)
            {
                data = obj;
                id = cmdCounter++;
            }
        }
    }
}
