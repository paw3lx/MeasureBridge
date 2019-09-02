using System;
using MeasureBridge.Model.Storage;
using MeasureBridge.StorageService.Extensions;
using Microsoft.Azure.WebJobs;
using Microsoft.Extensions.Logging;
using Microsoft.WindowsAzure.Storage.Table;

namespace MeasureBridge.IotHubFunctionApp
{
    public static class TimerReportFunctionApp
    {
        [FunctionName("TimerReportFunctionApp")]
        public static async void Run([TimerTrigger("0 * * * * *")]TimerInfo myTimer, [Table("ACCurrent")]CloudTable testTable, ILogger log)
        {
            log.LogInformation($"C# Timer trigger function executed at: {DateTime.Now}");

            // if (myTimer.IsPastDue)
            // {
            //     return;
            // }

            TableQuery<ACCurrent> query = new TableQuery<ACCurrent>().Take(1000);

            var segmentResult = testTable.ExecuteQuerySegmentedAsync(query, null);

            var result = await testTable.ExecuteQueryAsync(query);

            

        }
    }
}
