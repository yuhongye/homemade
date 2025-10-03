from fastapi import FastAPI
import uvicorn

from trace import traces_util

app = FastAPI()

traces_util.set_up()
traces_util.instrument(app)
from opentelemetry.trace import get_current_span



@app.post("/process")
async def process_text():
    span = get_current_span()
    print('span', span)
    with tracer.start_as_current_span("new-root-span", parent=None) as new_span:
        print('new span', new_span)
    return "response"

if __name__ == '__main__':
    uvicorn.run(app=app)
