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
        [return: Table("Consumption", Connection = "AzureWebJobsStorage")]
        public static async Task<Consumption> Run([TimerTrigger("0 */10 * * * *")]TimerInfo myTimer,
            [Table("ACCurrent")]CloudTable acCurrentTable,
            ILogger log)
        {
            log.LogInformation($"C# TimerConsumptionFunctionApp executed at: {DateTime.Now}");
            
            var currentKwh = await CalculatekWh(acCurrentTable);

            if (currentKwh == 0)
            {
                log.LogInformation($"Calculated kWh is 0");
                return null;
            }

            log.LogInformation($"Saved {currentKwh} to consumption collector");
            var entity = new Consumption()
            {
                kWh = currentKwh,
                PartitionKey = Guid.NewGuid().ToString(),
                RowKey = currentKwh.ToString()
            };

            return entity;
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
            
            double kWh = kpower * ((double)minutes / 60);

            return kWh;
        }
    }
}
