using System;
using System.Linq;
using System.Threading.Tasks;
using MeasureBridge.Model.Storage;
using MeasureBridge.StorageService.Extensions;
using Microsoft.Azure.WebJobs;
using Microsoft.Extensions.Logging;
using Microsoft.WindowsAzure.Storage.Table;

namespace MeasureBridge.IotHubFunctionApp
{
    public static class TimerConsumptionFunctionApp
    {
        [FunctionName("TimerConsumptionFunctionApp")]
        public static async void Run([TimerTrigger("0 */10 * * * *")]TimerInfo myTimer, [Table("ACCurrent")]CloudTable acCurrentTable, [Table("Consumption")]ICollector<Consumption> consumptionCollector,
            ILogger log, ExecutionContext context)
        {
            log.LogInformation($"C# TimerConsumptionFunctionApp executed at: {DateTime.Now}");
            
            var currentKwh = await CalculatekWh(acCurrentTable);

            var entity = new Consumption()
            {
                kWh = currentKwh,
                PartitionKey = System.Guid.NewGuid().ToString(),
                RowKey = currentKwh.ToString()
            };

            consumptionCollector.Add(entity);

            log.LogInformation($"Saved {currentKwh} to consumption collector");
        }

        private async static Task<double> CalculatekWh(CloudTable acCurrentTable)
        {
            int minutes = 10;
            DateTime from = DateTime.Now.AddMinutes(-minutes);

            var filter = TableQuery.CombineFilters(TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.GreaterThanOrEqual, from),
                TableOperators.And,
                TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.LessThanOrEqual, DateTime.Now));
            TableQuery<ACCurrent> query = new TableQuery<ACCurrent>().Where(filter);

            var result = await acCurrentTable.ExecuteQueryAsync(query);

            var averageAC = result.Any() ? result.Select(s => s.AC).Sum() / (12 * minutes) : 0;

            double kpower = (averageAC * 230) / 1000;
            
            double kWh = kpower * (minutes / 60);

            return kWh;
        }
    }
}
