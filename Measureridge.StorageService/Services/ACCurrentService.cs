using MeasureBridge.Model.Storage;
using MeasureBridge.StorageService.Interfaces;
using MeasureBridge.StorageService.Extensions;

using Microsoft.Extensions.Configuration;
using Microsoft.WindowsAzure.Storage;
using Microsoft.WindowsAzure.Storage.Table;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace MeasureBridge.StorageService.Services
{
    public class ACCurrentService : ITableService<ACCurrent>
    {
        private readonly CloudTableClient _tableClient;

        public ACCurrentService(IConfigurationRoot configuration)
        {
            var cloudStorageAccount = CloudStorageAccount.Parse(configuration.GetConnectionString("DefaultConnection"));
            _tableClient = cloudStorageAccount.CreateCloudTableClient();
        }

        public async Task<double> GetAverage(DateTime from, DateTime to)
        {
            CloudTable table = _tableClient.GetTableReference("ACCurrent");
            var filter = TableQuery.CombineFilters(TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.GreaterThanOrEqual, from),
                TableOperators.And,
                TableQuery.GenerateFilterConditionForDate("Timestamp", QueryComparisons.LessThanOrEqual, to));
            TableQuery<ACCurrent> query = new TableQuery<ACCurrent>().Where(filter);
            var segmentResult = await table.ExecuteQueryAsync(query);

            return segmentResult.Select(s => s.AC).Average();
        }

        public async Task<IEnumerable<ACCurrent>> GetEntities(int takeCount)
        {
            CloudTable table = _tableClient.GetTableReference("ACCurrent");
            TableQuery<ACCurrent> query = new TableQuery<ACCurrent>().Take(takeCount);

            var segmentResult = await table.ExecuteQueryAsync(query);

            return segmentResult;
        }
    }
}
