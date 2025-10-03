from opentelemetry import trace
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
from opentelemetry.propagate import inject

import asyncio
import os
from openai import AsyncOpenAI

# === 配置 OTLP 导出器 ===
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
client = AsyncOpenAI(base_url="http://127.0.0.1:8000", api_key="mock-apikey")

async def call_openai():
    with tracer.start_as_current_span("chat", context = None, kind=SpanKind.INTERNAL) as span:
        print('span', span)
        headers = {}
        inject(headers)
        response = await client.chat.completions.create(
            model="gpt-3.5-turbo",
            messages=[{"role": "user", "content": "Hello!"}],
            temperature=0.7,
            extra_headers=headers
        )
        print("OpenAI response:", response.choices[0].message.content)

asyncio.run(call_openai())