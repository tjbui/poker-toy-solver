import type { EquityRequest, EquityResult } from './lib/cards'

const API_BASE_URL = 'http://localhost:8080'

export async function calculateEquity(
  request: EquityRequest,
): Promise<EquityResult> {
  const response = await fetch(`${API_BASE_URL}/api/equity`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(request),
  })

  if (!response.ok) {
    throw new Error(`Backend request failed with status ${response.status}`)
  }

  return response.json()
}