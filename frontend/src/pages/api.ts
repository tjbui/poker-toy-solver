import type { EquityRequest, EquityResult } from '../lib/cards'

const API_BASE_URL = 'http://localhost:8080'

export async function calculateEquity(
  request: EquityRequest,
): Promise<EquityResult> {
  console.log('Equity request payload:', JSON.stringify(request, null, 2))

  const response = await fetch(`${API_BASE_URL}/api/equity`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(request),
  })

  const text = await response.text()

  if (!response.ok) {
    console.error('Backend error response:', text)
    throw new Error(
      `Backend request failed with status ${response.status}: ${text}`,
    )
  }

  return JSON.parse(text) as EquityResult
}