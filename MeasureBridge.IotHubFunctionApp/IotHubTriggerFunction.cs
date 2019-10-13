using IoTHubTrigger = Microsoft.Azure.WebJobs.EventHubTriggerAttribute;
using Microsoft.Azure.WebJobs;
using Microsoft.Azure.EventHubs;
using System.Text;
using Microsoft.Extensions.Logging;
using Newtonsoft.Json;
using MeasureBridge.IotHubFunctionApp.InputModel;
using MeasureBridge.Model.Storage;

namespace MeasureBridge.IotHubFunctionApp
{
    public static class IotHubTriggerFunction
    {
        [FunctionName("IotHubTriggerFunction")]
        public static void Run([IoTHubTrigger("messages/events", Connection = "ConnectionString")]EventData message, [Table("ACCurrent")]ICollector<ACCurrent> tableBinding, ILogger log)
        {
            var json = Encoding.UTF8.GetString(message.Body.Array);
            log.LogInformation($"C# IoT Hub trigger function processed a message: {json}");

            var m = JsonConvert.DeserializeObject<Message>(json);

            var entity = new ACCurrent()
            {
                AC = m.ACCurrent,
                PartitionKey = System.Guid.NewGuid().ToString(),
                RowKey = m.ACCurrent.ToString()
            };

            tableBinding.Add(entity);
        }
    }
}