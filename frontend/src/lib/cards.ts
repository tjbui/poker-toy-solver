export type Suit = 'S' | 'H' | 'C' | 'D'

export type Rank =
  | 'A'
  | 'K'
  | 'Q'
  | 'J'
  | 'T'
  | '9'
  | '8'
  | '7'
  | '6'
  | '5'
  | '4'
  | '3'
  | '2'

export type CardCode = `${Rank}${Suit}`
export type CardValue = CardCode | 'BACK' | null

export type CardSelectionTarget = {
  section: 'hero' | 'villain' | 'community'
  index: number
}

export type PlayerMode = 'exact' | 'range'
export type HandClass = string

export type PlayerRequest = {
  mode: PlayerMode
  cards: (CardCode | null)[]
  range: HandClass[]
}

export type EquityRequest = {
  hero: PlayerRequest
  villain: PlayerRequest
  community: (CardCode | null)[]
  simulations: number
  engine: 'cpu' | 'gpu'
}

export type EquityResult = {
  heroWinPct: number
  villainWinPct: number
  tiePct: number
  simulatedHands: number
  runtimeMs: number
  engine?: string
}

export const SUITS: Suit[] = ['S', 'H', 'C', 'D']

export const RANKS: Rank[] = [
  'A',
  'K',
  'Q',
  'J',
  'T',
  '9',
  '8',
  '7',
  '6',
  '5',
  '4',
  '3',
  '2',
]

export const ALL_CARDS: CardCode[] = SUITS.flatMap((suit) =>
  RANKS.map((rank) => `${rank}${suit}` as CardCode),
)

export function getSuitSymbol(suit: Suit): string {
  switch (suit) {
    case 'S':
      return '♠'
    case 'H':
      return '♥'
    case 'C':
      return '♣'
    case 'D':
      return '♦'
  }
}

export function getSuitClass(suit: Suit): string {
  return suit === 'H' || suit === 'D' ? 'suit-red' : 'suit-black'
}

export function getDisplayRank(rank: Rank): string {
  return rank === 'T' ? '10' : rank
}

export function getBackImage(): string {
  return new URL('../assets/card-back.png', import.meta.url).href
}

export function getCardRank(card: CardCode): Rank {
  return card[0] as Rank
}

export function getCardSuit(card: CardCode): Suit {
  return card[1] as Suit
}

export function normalizeCard(value: CardValue): CardCode | null {
  if (value === null || value === 'BACK') return null
  return value
}

export function buildExactPlayer(cards: CardValue[]): PlayerRequest {
  return {
    mode: 'exact',
    cards: cards.map(normalizeCard),
    range: [],
  }
}

export function buildEquityRequest(
  heroCards: CardValue[],
  villainCards: CardValue[],
  communityCards: CardValue[],
  simulations = 1000000,
  engine: 'cpu' | 'gpu' = 'cpu',
): EquityRequest {
  return {
    hero: buildExactPlayer(heroCards),
    villain: buildExactPlayer(villainCards),
    community: communityCards.map(normalizeCard),
    simulations,
    engine,
  }
}