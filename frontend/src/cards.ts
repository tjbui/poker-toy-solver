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

export type CardCode = `${Suit}-${Rank}`
export type CardValue = CardCode | 'BACK' | null

export type CardSelectionTarget = {
  section: 'hero' | 'villain' | 'community'
  index: number
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
  RANKS.map((rank) => `${suit}-${rank}` as CardCode),
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
  return new URL('./assets/card-back.png', import.meta.url).href
}