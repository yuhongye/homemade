from fastapi import FastAPI, Request
from fastapi.responses import StreamingResponse
import asyncio
import json

from opentelemetry import trace
from opentelemetry.instrumentation.fastapi import FastAPIInstrumentor
from opentelemetry.trace import TracerProvider, SpanKind
from opentelemetry.sdk.trace import ReadableSpan
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import (
    ConsoleSpanExporter,
    SimpleSpanProcessor,
    BatchSpanProcessor,
)
from opentelemetry.sdk.resources import Resource
from opentelemetry.trace import get_current_span
from opentelemetry.exporter.otlp.proto.http.trace_exporter import OTLPSpanExporter
from opentelemetry.propagate import inject, extract

otlp_exporter = ConsoleSpanExporter()

# === 配置 SDK Provider ===
resource = Resource.create({
    "service.name": "TW",
    "host.name": "TW-xxxx"
})
provider = TracerProvider(resource=resource)
trace.set_tracer_provider(provider)

span_processor = SimpleSpanProcessor(otlp_exporter)
provider.add_span_processor(span_processor)

tracer = trace.get_tracer(__name__)
app = FastAPI()
FastAPIInstrumentor.instrument_app(app)

@app.post("/chat/completions")
async def mock_chat_completions(request: Request):
    print('headers', request.headers)
    ctx = extract(request.headers)
    span = tracer.start_span("server", context=ctx)
    print('span', span)
    body = await request.json()
    stream = body.get("stream", False)

    if stream:
        async def fake_stream():
            chunks = [
                {
                    "id": "chatcmpl-mock1",
                    "object": "chat.completion.chunk",
                    "created": 1234567890,
                    "model": "gpt-3.5-turbo",
                    "choices": [{
                        "index": 0,
                        "delta": {"content": "Hello"},
                        "finish_reason": None
                    }]
                },
                {
                    "id": "chatcmpl-mock1",
                    "object": "chat.completion.chunk",
                    "created": 1234567891,
                    "model": "gpt-3.5-turbo",
                    "choices": [{
                        "index": 0,
                        "delta": {"content": " world"},
                        "finish_reason": None
                    }]
                },
                {
                    "id": "chatcmpl-mock1",
                    "object": "chat.completion.chunk",
                    "created": 1234567892,
                    "model": "gpt-3.5-turbo",
                    "choices": [{
                        "index": 0,
                        "delta": {},
                        "finish_reason": "stop"
                    }]
                }
            ]
            for chunk in chunks:
                yield f"data: {json.dumps(chunk)}\n\n"
                await asyncio.sleep(0.1)  # 模拟延迟
            yield "data: [DONE]\n\n"

        return StreamingResponse(fake_stream(), media_type="text/event-stream")

    # 非流式响应
    return {
        "id": "chatcmpl-mock1",
        "object": "chat.completion",
        "created": 1234567890,
        "model": "gpt-3.5-turbo",
        "choices": [{
            "index": 0,
            "message": {
                "role": "assistant",
                "content": "Hello world"
            },
            "finish_reason": "stop"
        }],
        "usage": {
            "prompt_tokens": 10,
            "completion_tokens": 5,
            "total_tokens": 15
        }
    }

if __name__ == '__main__':
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)