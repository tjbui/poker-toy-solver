import { useMemo, useState } from 'react'
import CardPickerModal from '../components/CardPickerModal'
import CardRow from '../components/CardRow'
import TableSection from '../components/TableSection'
import { calculateEquity } from '../pages/api'
import {
  buildEquityRequest,
  type CardCode,
  type CardSelectionTarget,
  type CardValue,
  type EquityResult,
} from '../lib/cards'

export default function EquityCalculatorPage() {
  const [heroCards, setHeroCards] = useState<CardValue[]>(['BACK', 'BACK'])
  const [villainCards, setVillainCards] = useState<CardValue[]>(['BACK', 'BACK'])
  const [communityCards, setCommunityCards] = useState<CardValue[]>([
    'BACK',
    'BACK',
    'BACK',
    'BACK',
    'BACK',
  ])

  const [activeTarget, setActiveTarget] = useState<CardSelectionTarget | null>(null)
  const [result, setResult] = useState<EquityResult | null>(null)
  const [isLoading, setIsLoading] = useState(false)

  const usedCards = useMemo(() => {
    return [...heroCards, ...villainCards, ...communityCards].filter(
      (card): card is CardCode => card !== null && card !== 'BACK',
    )
  }, [heroCards, villainCards, communityCards])

  async function handleSimulate() {
    setIsLoading(true)

    try {
      const request = buildEquityRequest(
        heroCards,
        villainCards,
        communityCards,
        100000,
        'cpu',
      )

      const data = await calculateEquity(request)
      setResult(data)
    } catch (err) {
      console.error(err)
      setResult(null)
    } finally {
      setIsLoading(false)
    }
  }

  function handleCardClick(section: CardSelectionTarget['section'], index: number) {
    setActiveTarget({ section, index })
  }

  function handleSelectCard(value: CardValue) {
    if (!activeTarget) return

    const { section, index } = activeTarget

    if (section === 'hero') {
      const next = [...heroCards]
      next[index] = value
      setHeroCards(next)
    } else if (section === 'villain') {
      const next = [...villainCards]
      next[index] = value
      setVillainCards(next)
    } else {
      const next = [...communityCards]
      next[index] = value
      setCommunityCards(next)
    }

    setActiveTarget(null)
  }

  return (
    <div className="page">
      <div className="table-shell">
        <div className="github-row">
          <p className="name">Tru Bui</p>
          <a
            href="https://github.com/tjbui/poker-equity"
            target="_blank"
            rel="noreferrer"
            className="github-link-icon"
          >
            <img
              src="https://cdn.jsdelivr.net/gh/devicons/devicon/icons/github/github-original.svg"
              alt="GitHub"
              className="github-logo"
            />
          </a>
        </div>

        <h1 className="title">Poker Equity Calculator</h1>

        <div className="table-layout">
          <div className="poker-section hero-section">
            <TableSection title="Hero Cards">
              <CardRow
                cards={heroCards}
                onCardClick={(index) => handleCardClick('hero', index)}
              />
            </TableSection>
          </div>

          <div className="poker-section villain-section">
            <TableSection title="Villain Cards">
              <CardRow
                cards={villainCards}
                onCardClick={(index) => handleCardClick('villain', index)}
              />
            </TableSection>
          </div>

          <div className="poker-section community-section">
            <TableSection title="Community Cards">
              <CardRow
                cards={communityCards}
                onCardClick={(index) => handleCardClick('community', index)}
              />
            </TableSection>
          </div>

          <div className="action-row">
            <button
              className="simulate-button"
              onClick={handleSimulate}
              disabled={isLoading}
              type="button"
            >
              {isLoading ? 'Running...' : 'Run Simulation'}
            </button>
          </div>

          <div className="results-section">
            <TableSection title="Results">
              {!result ? (
                <div className="results-list">
                  <p>Hero win percentage: NA</p>
                  <p>Villain win percentage: NA</p>
                  <p>Tie percentage: NA</p>
                  <p>_____ hands simulated in _____ms</p>
                </div>
              ) : (
                <div className="results-list">
                  <p>Hero win percentage: {result.heroWinPct.toFixed(2)}%</p>
                  <p>Villain win percentage: {result.villainWinPct.toFixed(2)}%</p>
                  <p>Tie percentage: {result.tiePct.toFixed(2)}%</p>
                  <p>
                    {result.simulatedHands.toLocaleString()} hands simulated in{' '}
                    {result.runtimeMs.toFixed(2)}ms
                  </p>
                </div>
              )}
            </TableSection>
          </div>
        </div>
      </div>

      <CardPickerModal
        isOpen={activeTarget !== null}
        usedCards={usedCards}
        onClose={() => setActiveTarget(null)}
        onSelect={handleSelectCard}
      />
    </div>
  )
}