using System;
using System.Configuration;
using System.Linq;
using MeasureBridge.Model.Storage;
using MeasureBridge.StorageService.Extensions;
using Microsoft.Azure.Devices;
using Microsoft.Azure.WebJobs;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Logging;
using Microsoft.WindowsAzure.Storage.Table;

namespace MeasureBridge.IotHubFunctionApp
{
    public static class TimerReportFunctionApp
    {
        [FunctionName("TimerReportFunctionApp")]
        public static async void Run([TimerTrigger("0 */5 * * * *")]TimerInfo myTimer, [Table("ACCurrent")]CloudTable testTable, ILogger log, ExecutionContext context)
        {
            log.LogInformation($"C# Timer trigger function executed at: {DateTime.Now}");

            if (myTimer.IsPastDue)
                return;

            DateTime from = DateTime.Now.AddHours(-1);

            var filter = TableQuery.CombineFilters(TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.GreaterThanOrEqual, from),
                TableOperators.And,
                TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.LessThanOrEqual, DateTime.Now));
            TableQuery<ACCurrent> query = new TableQuery<ACCurrent>().Where(filter);

            var result = await testTable.ExecuteQueryAsync(query);

            var average = result.Any() ? result.Select(s => s.AC).Average() : 0;

            var config = new ConfigurationBuilder()
             .SetBasePath(context.FunctionAppDirectory)
             .AddJsonFile("local.settings.json", optional: true, reloadOnChange: true)
             .AddEnvironmentVariables()
             .Build();

            using (var registryManager = RegistryManager.CreateFromConnectionString(config["DeviceConnectionString"]))
            {
                var deviceID = config["DeviceId"];
                var twin = await registryManager.GetTwinAsync(deviceID);
                twin.Properties.Desired["acAverageLastHour"] = average;
                await registryManager.UpdateTwinAsync(deviceID, twin, twin.ETag);
            }
        }
    }
}
