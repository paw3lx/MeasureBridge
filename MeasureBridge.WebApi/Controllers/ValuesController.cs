using System;
using System.Threading.Tasks;
using MeasureBridge.Model.Storage;
using MeasureBridge.StorageService.Interfaces;
using Microsoft.AspNetCore.Mvc;

namespace MeasureBridge.WebApi.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    public class ValuesController : ControllerBase
    {
        protected ITableService<ACCurrent> _service;
        public ValuesController(ITableService<ACCurrent> service)
        {
            _service = service;
        }

        // GET api/values
        [HttpGet]
        public async Task<ActionResult<double>> Get()
        {
            var from = DateTime.Parse("2019-08-23T18:34:12.8658488Z");
            var to = DateTime.Parse("2019-08-23T20:00:03.304601Z");
            var temp = await _service.GetAverage(from, to);
            return temp;
        }

        // GET api/values/5
        [HttpGet("{id}")]
        public ActionResult<string> Get(int id)
        {
            return "value";
        }

        // POST api/values
        [HttpPost]
        public void Post([FromBody] string value)
        {
        }

        // PUT api/values/5
        [HttpPut("{id}")]
        public void Put(int id, [FromBody] string value)
        {
        }

        // DELETE api/values/5
        [HttpDelete("{id}")]
        public void Delete(int id)
        {
        }
    }
}
