using System;
using System.Linq;
using System.Threading.Tasks;
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
        public static async void Run([TimerTrigger("0 */5 * * * *")]TimerInfo myTimer, [Table("ACCurrent")]CloudTable testTable, [Table("Consumption")]CloudTable consumptionTable,
            ILogger log, ExecutionContext context)
        {
            log.LogInformation($"C# Timer trigger function executed at: {DateTime.Now}");

            var average = await GetAverageAc(testTable);
            (double kWhToday, double kWhLast7Days, double kWhLastMonth) = await GetkWhStats(consumptionTable);

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
                twin.Properties.Desired["kWhToday"] = kWhToday;
                twin.Properties.Desired["kWhLast7Days"] = kWhLast7Days;
                twin.Properties.Desired["kWhLastMonth"] = kWhLastMonth;
                await registryManager.UpdateTwinAsync(deviceID, twin, twin.ETag);
            }
        }

        private async static Task<double> GetAverageAc(CloudTable acCurrentTable)
        {
            DateTime from = DateTime.Now.AddHours(-1);

            var filter = TableQuery.CombineFilters(TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.GreaterThanOrEqual, from),
                TableOperators.And,
                TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.LessThanOrEqual, DateTime.Now));
            TableQuery<ACCurrent> query = new TableQuery<ACCurrent>().Where(filter);

            var result = await acCurrentTable.ExecuteQueryAsync(query);

            var average = result.Any() ? result.Select(s => s.AC).Sum() / (12 * 60) : 0;
            
            return average;
        }

        private async static Task<Tuple<double, double, double>> GetkWhStats(CloudTable consumptionTable)
        {
            DateTime from = DateTime.Now.AddMonths(-1);

            var filter = TableQuery.CombineFilters(TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.GreaterThanOrEqual, from),
                TableOperators.And,
                TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.LessThanOrEqual, DateTime.Now));
            TableQuery<Consumption> query = new TableQuery<Consumption>().Where(filter);

            var result = await consumptionTable.ExecuteQueryAsync(query);

            var today = result.Where(r => r.Timestamp >= DateTime.Today).Select(s => s.kWh).DefaultIfEmpty().Sum();
            var last7Days = result.Where(r => r.Timestamp >= DateTime.Today.AddDays(-7)).Select(s => s.kWh).DefaultIfEmpty().Sum();
            var lastMonth = result.Select(s => s.kWh).DefaultIfEmpty().Sum();
            
            return Tuple.Create(today, last7Days, lastMonth);
        }
    }
}
