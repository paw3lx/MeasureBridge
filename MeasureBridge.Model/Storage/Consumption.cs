using Microsoft.WindowsAzure.Storage.Table;
using System;

namespace MeasureBridge.Model.Storage
{
    public class Consumption : TableEntity
    {
        public double kWh { get; set; }
    }
}
