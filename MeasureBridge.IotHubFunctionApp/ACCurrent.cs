using Microsoft.WindowsAzure.Storage.Table;

namespace MeasureBridge.IotHubFunctionApp
{
    public class ACCurrent : TableEntity
    {
        public double AC { get; set; }
    }
}
