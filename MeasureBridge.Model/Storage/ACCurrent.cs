using Microsoft.WindowsAzure.Storage.Table;
using System;

namespace MeasureBridge.Model.Storage
{
    public class ACCurrent : TableEntity
    {
        public double AC { get; set; }
    }
}
